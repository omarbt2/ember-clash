#include "Gameplay/Match.h"

#include "Core/Config.h"
#include "Core/Logger.h"
#include "Gameplay/AiController.h"
#include "Gameplay/Attack.h"
#include "Gameplay/CombatConfig.h"
#include "Gameplay/Fighter.h"
#include "Gameplay/InputMap.h"
#include "Graphics/Renderer.h"
#include "Input/InputManager.h"
#include "UI/Hud.h"

#include <cmath>
#include <cstdlib>

namespace ember {

namespace {

// True iff two AABBs overlap (touching counts). Half-open in the inclusive
// direction; the smash-style "edge-tap" feel of touching = hit is desired
// here. The existing arena collision uses the same convention.
bool aabb_overlap(const Aabb& a, const Aabb& b) {
    return a.x < b.x + b.w
        && a.x + a.w > b.x
        && a.y < b.y + b.h
        && a.y + a.h > b.y;
}

// Given an attacker whose active hitbox overlaps a target's hurtbox, compute
// the per-hit response (knockback velocity + hitstun) and apply it. The
// caller is responsible for verifying the overlap and the attacker's
// `attack_has_hit_` flag; this function does not check them.
//
// Returns true if the hit actually landed (target wasn't invulnerable), so
// Match can record a hit-spark at the impact point.
bool apply_attack(Fighter& attacker, Fighter& target) {
    const Attack& atk = get_attack(attacker.current_attack_id());

    // Knockback formula per the design brief \u00a713.
    //     knockback = base * (1 + damage / divisor)
    const int damage = target.damage_percent();
    const double kb = static_cast<double>(atk.base_knockback)
                    * (1.0
                       + static_cast<double>(damage)
                         / static_cast<double>(combat::kKnockbackDamageDivisor));

    const double sx = static_cast<double>(atk.knockback_angle_x) * static_cast<double>(attacker.facing());
    const double sy = static_cast<double>(atk.knockback_angle_y);
    const double mag = std::sqrt(sx * sx + sy * sy);
    const double nx  = (mag > 1e-6) ? sx / mag : sx;
    const double ny  = (mag > 1e-6) ? sy / mag : sy;

    const Vec2 knockback_velocity{ nx * kb, ny * kb };

    // Pre-hit record so we can detect whether the target rejected the hit
    // (invulnerability) before mutating anything.
    const bool was_invuln = target.is_invulnerable();
    target.apply_hit(atk.damage, knockback_velocity, atk.hitstun_frames);
    return !was_invuln && !target.is_dead();
}

// Midpoint of the AABB. Used for hit-spark placement.
Vec2 aabb_center(const Aabb& a) {
    return Vec2{ a.x + a.w * 0.5f, a.y + a.h * 0.5f };
}

// M6-C: deterministic shake offset. std::rand seeded in main()'s App
// startup so the shake is the same across runs in a given build. Returns
// a value in [-mag, +mag].
float shake_offset_1d(float mag) {
    const float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);  // [0, 1]
    return (r * 2.0f - 1.0f) * mag;
}

} // namespace

Match::Match()
    : respawn_timer_{ -1.0, -1.0 } {
    reset();
    // M4 default: P1 is human-controlled, P2 is the bot. The future
    // difficulty-select screen will swap this; the harness can override it
    // via set_ai() before each tick.
    ai_[1] = std::make_unique<AiController>();
}

Match::~Match() = default;

void Match::reset() {
    arena_.reset_default();
    for (int i = 0; i < kFighterCount; ++i) {
        fighters_[i] = std::make_unique<Fighter>(i, arena_);
        respawn_timer_[i] = -1.0;
    }
    // AI controllers are not owned by the fighters; reset their internal
    // state but keep them attached so a rematch keeps the same difficulty.
    for (int i = 0; i < kFighterCount; ++i) {
        if (ai_[i]) ai_[i]->reset_for_test();
    }
    ko_banner_timer_ = 0.0;
    phase_           = Phase::Fighting;
    last_hit_        = HitEvent{0.0f, 0.0f, false};
    hit_spark_age_   = 0;
}

void Match::set_ai(int fighter_index, std::unique_ptr<AiController> controller) {
    if (fighter_index < 0 || fighter_index >= kFighterCount) return;
    ai_[fighter_index] = std::move(controller);
}

bool Match::is_ai(int fighter_index) const {
    if (fighter_index < 0 || fighter_index >= kFighterCount) return false;
    return ai_[fighter_index] != nullptr;
}

const AiController* Match::ai(int fighter_index) const {
    if (fighter_index < 0 || fighter_index >= kFighterCount) return nullptr;
    return ai_[fighter_index].get();
}

void Match::update(const InputManager& input, double dt) {
    // M6-C: tick the post-KO shake at the TOP of update so the first
    // frame after the match-end transition (which sets ko_shake_frames_
    // to kKoShakeFrames) does not also decrement it. Without this, the
    // first observable value is kKoShakeFrames-1, which is awkward for
    // tests and inconsistent with how hit-stop's first observable value
    // is kHitStopFrames (not decremented until the *next* update).
    if (ko_shake_frames_ > 0) {
        --ko_shake_frames_;
        const float t = static_cast<float>(ko_shake_frames_)
                      / static_cast<float>(combat::kKoShakeFrames);
        const float mag = combat::kKoShakeMagnitude * t;
        ko_shake_offset_x_ = shake_offset_1d(mag);
        ko_shake_offset_y_ = shake_offset_1d(mag);
    } else {
        ko_shake_offset_x_ = 0.0f;
        ko_shake_offset_y_ = 0.0f;
    }

    // M5: when the match is over, freeze all fighter motion and just let
    // the KO banner timer tick. The rematch reset comes from App when the
    // player presses R.
    if (phase_ == Phase::MatchOver) {
        ko_banner_timer_ += dt;
        if (hit_spark_age_ > 0) --hit_spark_age_;
        return;
    }

    // M6-C: hit-stop. While a hit is "fresh" the match freezes - motion
    // and AI are paused so the hit registers with weight, but the hit-
    // spark still ages. We still want input sampled (the player may be
    // buffering an input for the moment the freeze ends), so we update
    // AI/motion ONLY when not in hit-stop.
    if (hit_stop_frames_ > 0) {
        --hit_stop_frames_;
        if (hit_spark_age_ > 0) --hit_spark_age_;
        return;
    }

    // 1) Death-zone detection (pre-update). M5: Match owns the alive->dead
    //    transition so the stock decrement + respawn timer kick in in the
    //    same frame the fighter falls off. Done before Fighter::update so
    //    a fighter that was already past the death zone at frame start
    //    gets the bookkeeping this frame instead of next.
    for (int i = 0; i < kFighterCount; ++i) {
        Fighter& f = *fighters_[i];
        if (!f.is_dead() && f.body().y > arena_.death_zone_y()) {
            f.set_dead_for_test(true);
            if (f.stocks() > 0) {
                f.set_stocks_for_test(f.stocks() - 1);
            }
            if (f.stocks() > 0) {
                respawn_timer_[i] = combat::kRespawnDelaySeconds;
            }
        }
    }

    // 2) Drive each fighter. AI-controlled fighters ask their controller
    //    for intents; everyone else uses the keyboard. Dead fighters are
    //    skipped here; their respawn timer ticks down in step 4.
    for (int i = 0; i < kFighterCount; ++i) {
        Fighter& self = *fighters_[i];
        if (self.is_dead()) continue;

        PlayerIntent motion{};
        CombatIntent combat{};
        if (ai_[i]) {
            const int opp_idx = (i == 0) ? 1 : 0;
            const Fighter& opp = *fighters_[opp_idx];
            ai_[i]->decide(self, opp, arena_, dt, motion, combat);
        } else {
            const InputMap::Sample s = InputMap::sample(input, i);
            motion = s.motion;
            combat = s.combat;
        }
        self.update(motion, combat, arena_, dt);
    }

    // 3) Resolve hits: for each ordered pair (attacker, defender) where the
    //    attacker's current attack is in its active window and the hitbox
    //    overlaps the defender's hurtbox, apply the hit.
    for (int a = 0; a < kFighterCount; ++a) {
        Fighter& attacker = *fighters_[a];
        const Aabb hb = attacker.active_hitbox();
        if (hb.w <= 0.0f || hb.h <= 0.0f) continue;

        for (int d = 0; d < kFighterCount; ++d) {
            if (d == a) continue;
            Fighter& target = *fighters_[d];
            if (target.is_dead()) continue;
            if (aabb_overlap(hb, target.body())) {
                if (apply_attack(attacker, target)) {
                    const Vec2 c = aabb_center(hb);
                    last_hit_      = HitEvent{
                        static_cast<float>(c.x),
                        static_cast<float>(c.y),
                        true
                    };
                    hit_spark_age_ = combat::kHitSparkFrames;
                    // M6-C: a successful hit pauses the simulation for a
                    // few frames so the impact reads. The pause is set
                    // here (not in apply_attack) because a hit on an
                    // invulnerable or already-dead target does NOT
                    // trigger the stop.
                    hit_stop_frames_ = combat::kHitStopFrames;
                }
            }
        }
    }

    // 4) Respawn tick: when a fighter's timer reaches 0, respawn them and
    //    apply the post-respawn invulnerability window. Skipped once the
    //    fighter's stocks have hit 0 - they stay off-stage and the match
    //    ends.
    for (int i = 0; i < kFighterCount; ++i) {
        if (respawn_timer_[i] < 0.0) continue;
        respawn_timer_[i] -= dt;
        if (respawn_timer_[i] <= 0.0) {
            respawn_timer_[i] = -1.0;
            fighters_[i]->respawn(arena_);
            fighters_[i]->set_respawn_invuln_seconds(
                combat::kRespawnInvulnSeconds);
        }
    }

    // 5) Match-end check: when at least one fighter has 0 stocks, the
    //    match is over.
    bool any_out = false;
    for (int i = 0; i < kFighterCount; ++i) {
        if (fighters_[i]->stocks() <= 0) {
            any_out = true;
            break;
        }
    }
    if (any_out) {
        phase_           = Phase::MatchOver;
        ko_banner_timer_ = 0.0;
        // M6-C: arm the post-KO shake. The actual offset is set
        // immediately to a fresh random value at full magnitude, then
        // ticked at the top of subsequent updates. We initialize the
        // offset here so the very first frame the renderer reads (this
        // same frame, via App::render) shows a meaningful shake rather
        // than the zero default.
        ko_shake_frames_ = combat::kKoShakeFrames;
        const float mag = combat::kKoShakeMagnitude;
        ko_shake_offset_x_ = shake_offset_1d(mag);
        ko_shake_offset_y_ = shake_offset_1d(mag);
    }

    // 6) Hit-spark aging (the normal, non-frozen tick). The shake is
    // ticked at the top of update so the post-KO frame doesn't both
    // arm and decay it.
    if (hit_spark_age_ > 0) --hit_spark_age_;
}

void Match::render(Renderer& renderer) const {
    arena_.render(renderer);
    for (int i = 0; i < kFighterCount; ++i) {
        fighters_[i]->render(renderer);
    }
    // M6-C: M3's hurtbox debug outline is gone - the hit-spark at the
    // last_hit_ position now communicates the same info at the moment
    // the player cares about it. The per-fighter hurtbox still lives in
    // Fighter::body() and is used by Match's hit resolver, so this is a
    // render-only cleanup.
    //
    // M5 HUD: per-fighter damage-floats above their heads, then the
    // fighter cards in the top corners.
    for (int i = 0; i < kFighterCount; ++i) {
        ui::Hud::draw_damage_floats(renderer, *fighters_[i]);
    }
    ui::Hud::draw_fighter_card(renderer, *fighters_[0], 0);
    ui::Hud::draw_fighter_card(renderer, *fighters_[1], 1);

    // Hit-spark overlay.
    if (hit_spark_age_ > 0 && last_hit_.valid) {
        ui::Hud::draw_hit_spark(renderer, last_hit_.x, last_hit_.y,
                                hit_spark_age_);
    }

    // KO banner last so it sits on top of everything else.
    ui::Hud::draw_ko_banner(renderer, *this, *fighters_[0], *fighters_[1]);
}

const Fighter& Match::fighter(int index) const {
    return *fighters_[index];
}

} // namespace ember
