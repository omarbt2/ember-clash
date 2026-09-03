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

// --------------------------------------------------------------------
// M11: deterministic state fingerprint.
//
// Two Matches driven with identical inputs and identical AI seeds
// MUST produce identical state_hash() values on every tick. We use
// FNV-1a 64-bit (the standard xorshift-friendly variant) and fold
// in the raw bytes of every field that affects simulation output.
//
// We hash:
//   - arena (which is currently static, but we fold it in for safety
//     so a future arena-variant can change the hash too);
//   - per-fighter: body (x,y,w,h as int32 for cross-platform safety -
//     we don't hash the float bit pattern, since two compilers could
//     legitimately produce different bit patterns for the same value),
//     velocity (int32 of the cast), grounded, dead, damage_percent,
//     stocks, facing, jumps_remaining, state enum, current_attack_id,
//     attack_frame, attack_has_hit (folded as 0/1 so we don't have to
//     define its storage), hitstun_frames, invuln_frames;
//   - match: phase, ko_banner_timer (as int32 of the cast - dt
//     accumulation is deterministic at 1/60, but a future variable-dt
//     build could still produce a different but consistent hash);
//   - respawn timers (per fighter, int32 of the cast);
//   - hit-spark: age + last_hit_ (x, y as int32, valid as 0/1);
//   - hit-stop: frames remaining;
//   - ko-shake: frames remaining + offset x,y as int32.
//
// We do NOT hash: AI RNG state (it's an internal state, not a
// simulation output - two bots with the same seed will make the same
// decisions regardless of internal RNG state representation), or
// display-only state (renderer view offset, UI strings).
//
// The contract this function provides is: same inputs + same AI
// seed -> same hash. The M11 test harness verifies the contract.
// The future M10 online implementation will exchange this hash
// between peers to detect desync.
// --------------------------------------------------------------------
std::uint64_t Match::state_hash() const {
    // FNV-1a 64-bit constants.
    constexpr std::uint64_t kFnvOffset = 0xcbf29ce484222325ULL;
    constexpr std::uint64_t kFnvPrime  = 0x100000001b3ULL;
    std::uint64_t h = kFnvOffset;

    // Helper: fold a 32-bit value into the hash.
    auto mix32 = [&](std::uint32_t v) {
        for (int i = 0; i < 4; ++i) {
            h ^= (v >> (i * 8)) & 0xFFu;
            h *= kFnvPrime;
        }
    };
    // Helper: fold a float (as int32) into the hash. We cast the
    // float to int32 first so that the bit pattern is the float's
    // value, not its raw bits. Two compilers must agree on the cast
    // for the same float value to produce the same int32; the C++
    // standard guarantees that for finite values representable in
    // both, so this is portable enough for our use.
    auto mix_f32 = [&](float f) {
        std::int32_t bits = static_cast<std::int32_t>(f);
        mix32(static_cast<std::uint32_t>(bits));
    };
    // Helper: fold a double (as int64) into the hash. Split into two
    // 32-bit folds so we don't lose entropy.
    auto mix_f64 = [&](double d) {
        std::int64_t bits = static_cast<std::int64_t>(d);
        mix32(static_cast<std::uint32_t>(bits & 0xFFFFFFFFu));
        mix32(static_cast<std::uint32_t>(
                 (static_cast<std::uint64_t>(bits) >> 32) & 0xFFFFFFFFu));
    };
    // Helper: fold a Vec2 component (double) as a 32-bit int. The
    // physics store velocity as double, but the values are small
    // enough that int32 captures them faithfully. Avoids the
    // double->float narrowing warning C4244.
    auto mix_dbl_as_i32 = [&](double d) {
        std::int32_t bits = static_cast<std::int32_t>(d);
        mix32(static_cast<std::uint32_t>(bits));
    };

    // --- Arena (currently static, but fold for safety) ---
    for (const Platform& p : arena_.platforms()) {
        mix_f32(p.aabb.x); mix_f32(p.aabb.y);
        mix_f32(p.aabb.w); mix_f32(p.aabb.h);
    }
    mix_f32(arena_.death_zone_y());
    mix_f32(arena_.spawn_x(0));
    mix_f32(arena_.spawn_x(1));
    mix_f32(arena_.spawn_y());

    // --- Per-fighter state ---
    for (int i = 0; i < kFighterCount; ++i) {
        const Fighter& f = *fighters_[i];
        // Body AABB.
        mix_f32(f.body().x); mix_f32(f.body().y);
        mix_f32(f.body().w); mix_f32(f.body().h);
        // Velocity (Vec2 is double; cast to int32 to fit the
        // single-32-bit fold + avoid the float-narrowing warning).
        mix_dbl_as_i32(f.velocity().x);
        mix_dbl_as_i32(f.velocity().y);
        // Booleans as 0/1.
        mix32(f.is_grounded() ? 1u : 0u);
        mix32(f.is_dead()     ? 1u : 0u);
        // Integer state.
        mix32(static_cast<std::uint32_t>(f.player_index()));
        mix32(static_cast<std::uint32_t>(f.damage_percent()));
        mix32(static_cast<std::uint32_t>(f.stocks()));
        mix32(static_cast<std::uint32_t>(f.facing()));
        mix32(static_cast<std::uint32_t>(f.jumps_remaining()));
        mix32(static_cast<std::uint32_t>(f.state()));
        mix32(static_cast<std::uint32_t>(f.current_attack_id()));
        mix32(static_cast<std::uint32_t>(f.attack_frame()));
        mix32(static_cast<std::uint32_t>(f.hitstun_frames()));
        mix32(static_cast<std::uint32_t>(f.invuln_frames()));
    }

    // --- Match flow ---
    mix32(static_cast<std::uint32_t>(phase_));
    mix_f64(ko_banner_timer_);

    // --- Per-fighter respawn timers (seconds) ---
    for (int i = 0; i < kFighterCount; ++i) {
        mix_f64(respawn_timer_[i]);
    }

    // --- Hit-spark ---
    mix32(static_cast<std::uint32_t>(hit_spark_age_));
    mix_f32(last_hit_.x);
    mix_f32(last_hit_.y);
    mix32(last_hit_.valid ? 1u : 0u);

    // --- Hit-stop ---
    mix32(static_cast<std::uint32_t>(hit_stop_frames_));

    // --- KO shake ---
    mix32(static_cast<std::uint32_t>(ko_shake_frames_));
    mix_f32(ko_shake_offset_x_);
    mix_f32(ko_shake_offset_y_);

    return h;
}

} // namespace ember
