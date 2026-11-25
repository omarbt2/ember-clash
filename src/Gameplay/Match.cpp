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

// Resolve one active swing against one hurtbox. Defence is resolved by the
// defender (guard/parry/dodge/invulnerability), not bypassed by the match
// loop. Any non-ignored contact consumes the attack's one-hit budget.
Fighter::HitOutcome apply_attack(Fighter& attacker, Fighter& target) {
    if (attacker.attack_has_hit()) return Fighter::HitOutcome::Ignored;

    const Attack& atk = get_attack(attacker.current_attack_id());
    const double damage_scale = attacker.surge_active()
        ? combat::kSurgeDamageMul : 1.0;
    const double knockback_scale = attacker.surge_active()
        ? combat::kSurgeKnockbackMul : 1.0;
    const int scaled_damage = static_cast<int>(std::lround(atk.damage * damage_scale));

    // knockback = base * (1 + target_damage / divisor), then Surge scaling.
    const double kb = static_cast<double>(atk.base_knockback)
                    * (1.0 + static_cast<double>(target.damage_percent())
                         / static_cast<double>(combat::kKnockbackDamageDivisor))
                    * knockback_scale;

    const double sx = static_cast<double>(atk.knockback_angle_x)
                    * static_cast<double>(attacker.facing());
    const double sy = static_cast<double>(atk.knockback_angle_y);
    const double mag = std::sqrt(sx * sx + sy * sy);
    const double nx  = (mag > 1e-6) ? sx / mag : sx;
    const double ny  = (mag > 1e-6) ? sy / mag : sy;
    const Vec2 knockback_velocity{nx * kb, ny * kb};

    const Fighter::HitOutcome outcome = target.receive_hit(
        scaled_damage, knockback_velocity, atk.hitstun_frames, attacker.facing());
    if (outcome == Fighter::HitOutcome::Ignored) return outcome;

    // One contact per swing, including a blocked, parried, or dodged swing.
    // Parry stun resets the attack state, so mark after stun_for() to keep a
    // stale precomputed hitbox from resolving against another target in the
    // same match tick (important when the roster grows beyond two fighters).
    if (outcome == Fighter::HitOutcome::Parried) {
        attacker.stun_for(combat::kParryAttackerStun);
        attacker.mark_attack_hit();
    } else {
        attacker.mark_attack_hit();
    }
    if (outcome == Fighter::HitOutcome::Hit) {
        attacker.add_surge(scaled_damage * combat::kSurgeGainPerDamageDealt);
    }
    return outcome;
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
    arena_.reset(arena_id_);
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
    // P15: start the intro sequence (READY/FIGHT). The first frame
    // after reset shows "READY..."; the banner transitions to "FIGHT!"
    // after kIntroBannerReadyFrames and clears after
    // kIntroBannerReadyFrames + kIntroBannerFightFrames. AI physics
    // is gated on intro_banner_state_ == 2 in Match::update.
    match_startup_ticks_ = 0;
    intro_banner_state_  = 0;
    live_ticks_          = 0;
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

AiController* Match::ai_mutable(int fighter_index) {
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

    // P15: intro sequence gate. The very first
    // (kIntroBannerReadyFrames + kIntroBannerFightFrames) frames of
    // every match (and rematch) are intro frames: physics, AI, and
    // hit resolution are FROZEN so the player can see the spawn
    // positions and react to the "READY..." / "FIGHT!" banners
    // without the bot immediately running in. During this window
    // we still tick match_startup_ticks_ and recompute
    // intro_banner_state_ so the Hud can render the right banner,
    // and we still age any active hit-spark so a hit that landed
    // the previous frame does not freeze on screen. We also let
    // the KO shake tick down (a held shake from a prior match's
    // last hit would otherwise linger into the next match, but
    // reset() clears ko_shake_frames_ to 0 so this is a no-op in
    // practice).
    if (intro_banner_state_ != 2) {
        ++match_startup_ticks_;
        if (match_startup_ticks_ < combat::kIntroBannerReadyFrames) {
            intro_banner_state_ = 0;  // "READY..."
        } else if (match_startup_ticks_ <
            combat::kIntroBannerReadyFrames
                + combat::kIntroBannerFightFrames) {
            intro_banner_state_ = 1;  // "FIGHT!"
        } else {
            // Intro done; mark state 2 ("live"). The match has not
            // been "playing" yet - the first frame of live play is
            // the next call to update(). The AI's
            // kAiStartupGraceTicks gate is consulted inside
            // AiController, which reads match_startup_ticks_ to
            // decide whether the grace window is still active.
            intro_banner_state_ = 2;
        }
        if (hit_spark_age_ > 0) --hit_spark_age_;
        return;
    }

    // P15: we are now in live play. Increment the monotonic live
    // play age counter that downstream systems (AI startup grace,
    // future "match has been live for N seconds" logic) use to
    // schedule post-intro behaviour. match_startup_ticks_ is
    // frozen at this point, so live_ticks_ is the only correct
    // counter for the "frames since play started" question.
    ++live_ticks_;

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
        if (!f.is_dead() && arena_.is_out_of_bounds(f.body())) {
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
            ai_[i]->decide(self, opp, arena_, dt,
                           live_ticks_,
                           motion, combat);
        } else {
            if (i == 0 && training_recorder_.playing()) {
                training::RecordedFrame frame{};
                if (training_recorder_.next(frame)) {
                    motion = frame.motion;
                    combat = frame.combat;
                }
            } else {
                const InputMap::Sample s = InputMap::sample(input, i);
                motion = s.motion;
                combat = s.combat;
                if (i == 0 && training_recorder_.recording()) {
                    training_recorder_.record(motion, combat);
                }
            }
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
                const Fighter::HitOutcome outcome = apply_attack(attacker, target);
                if (outcome != Fighter::HitOutcome::Ignored) {
                    const Vec2 c = aabb_center(hb);
                    last_hit_ = HitEvent{static_cast<float>(c.x),
                                         static_cast<float>(c.y), true};
                    hit_spark_age_ = combat::kHitSparkFrames;
                    // A clean hit gets full hit-stop. Defensive contact still
                    // gets a spark without freezing the simulation.
                    if (outcome == Fighter::HitOutcome::Hit) {
                        hit_stop_frames_ = combat::kHitStopFrames;
                    }
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

    // P15: intro banner is drawn after the hit-spark so it sits
    // on top. draw_intro_banner() is a no-op once the intro is
    // over, so we can call it unconditionally here.
    ui::Hud::draw_intro_banner(renderer, *this);

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

    // --- Arena data ---
    mix32(static_cast<std::uint32_t>(arena_.id()));
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

    // --- Training recording / playback state ---
    mix32(static_cast<std::uint32_t>(training_recorder_.size()));
    mix32(static_cast<std::uint32_t>(training_recorder_.cursor()));
    mix32(training_recorder_.recording() ? 1u : 0u);
    mix32(training_recorder_.playing() ? 1u : 0u);
    mix32(training_recorder_.overflowed() ? 1u : 0u);
    for (std::size_t i = 0; i < training_recorder_.size(); ++i) {
        const training::RecordedFrame* frame = training_recorder_.frame_at(i);
        if (!frame) continue;
        const PlayerIntent& m = frame->motion;
        const CombatIntent& c = frame->combat;
        mix32(m.move_left ? 1u : 0u); mix32(m.move_right ? 1u : 0u);
        mix32(m.jump_pressed ? 1u : 0u); mix32(m.fast_fall_held ? 1u : 0u);
        mix32(m.up_held ? 1u : 0u); mix32(m.run_held ? 1u : 0u);
        mix32(c.jab_pressed ? 1u : 0u); mix32(c.ftilt_pressed ? 1u : 0u);
        mix32(c.nair_pressed ? 1u : 0u); mix32(c.special_pressed ? 1u : 0u);
        mix32(c.smash_held ? 1u : 0u); mix32(c.shield_held ? 1u : 0u);
        mix32(c.dodge_pressed ? 1u : 0u); mix32(c.surge_pressed ? 1u : 0u);
    }

    // --- Match flow ---
    mix32(static_cast<std::uint32_t>(phase_));
    mix_f64(ko_banner_timer_);

    // --- P15 intro sequence ---
    // match_startup_ticks_ is sim-affecting (controls when AI
    // starts committing, when the banner clears, when the
    // "live" state begins). intro_banner_state_ is purely a
    // display field derived from it; folding both is cheap
    // and keeps the hash self-documenting. live_ticks_ is
    // the monotonic live-play age counter used by the AI
    // grace gate and the post-attack cooldown, so it is
    // also sim-affecting and must be folded in.
    mix32(static_cast<std::uint32_t>(match_startup_ticks_));
    mix32(static_cast<std::uint32_t>(intro_banner_state_));
    mix32(static_cast<std::uint32_t>(live_ticks_));

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
