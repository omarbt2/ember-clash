
#include "Gameplay/Match.h"

#include "Core/Config.h"
#include "Core/Logger.h"
#include "Gameplay/AiController.h"
#include "Gameplay/Attack.h"
#include "Data/HitboxEngine.h"
#include "Data/MoveContent.h"
#include "Gameplay/CombatConfig.h"
#include "Gameplay/Fighter.h"
#include "Gameplay/InputMap.h"
#include "Graphics/Renderer.h"
#include "Input/InputManager.h"
#include "UI/Hud.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>

#include <bit>
#include <cmath>
#include <limits>
#include <string>

namespace ember {

namespace {

// Resolve one active content hitbox against one hurtbox. Defence is resolved
// by the defender (guard/parry/dodge/invulnerability), not bypassed by Match.
// Any non-ignored contact consumes the current attack's one-hit budget.
Fighter::HitOutcome apply_attack(Fighter& attacker, Fighter& target,
                                 const data::ActiveHitbox& hitbox) {
    if (attacker.attack_has_hit()) return Fighter::HitOutcome::Ignored;

    const double damage_scale = attacker.surge_active()
        ? combat::kSurgeDamageMul : 1.0;
    const double knockback_scale = attacker.surge_active()
        ? combat::kSurgeKnockbackMul : 1.0;
    const int scaled_damage = static_cast<int>(std::lround(hitbox.damage * damage_scale));

    // knockback = base * (1 + target_damage / divisor), then Surge scaling.
    // P3 content formula: base + scaled damage growth. The roster's per-fighter
    // weight table is not implemented yet, so runtime currently uses the neutral
    // reference weight (100); the formula remains explicit for later data hookup.
    constexpr double kReferenceWeight = 100.0;
    const double damage_after_hit = static_cast<double>(target.damage_percent()
                                                        + scaled_damage);
    const double damage_growth = static_cast<double>(hitbox.knockback_scale)
        * damage_after_hit / static_cast<double>(combat::kKnockbackDamageDivisor)
        * (100.0 / kReferenceWeight);
    const double kb = (static_cast<double>(hitbox.base_knockback) + damage_growth)
                    * knockback_scale;

    // Content angle convention: degrees, 0 = forward, 90 = upward, 270 = downward.
    const double radians = static_cast<double>(hitbox.angle % 360)
                        * 3.14159265358979323846 / 180.0;
    const double sx = std::cos(radians) * static_cast<double>(attacker.facing());
    const double sy = -std::sin(radians);
    const double mag = std::sqrt(sx * sx + sy * sy);
    const double nx  = (mag > 1e-6) ? sx / mag : sx;
    const double ny  = (mag > 1e-6) ? sy / mag : sy;
    const Vec2 knockback_velocity{nx * kb, ny * kb};

    const Fighter::HitOutcome outcome = target.receive_hit(
        scaled_damage, knockback_velocity, hitbox.hitstun, attacker.facing());
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

const char* move_id_for_attack(AttackId id) {
    switch (id) {
    case AttackId::Jab: return "jab";
    case AttackId::ForwardTilt: return "ftilt";
    case AttackId::NeutralAir: return "nair";
    case AttackId::UpTilt: return "utilt";
    case AttackId::DownTilt: return "dtilt";
    case AttackId::ForwardSmash: return "fsmash";
    case AttackId::UpSmash: return "usmash";
    case AttackId::DownSmash: return "dsmash";
    case AttackId::ForwardAir: return "fair";
    case AttackId::BackAir: return "bair";
    case AttackId::UpAir: return "uair";
    case AttackId::DownAir: return "dair";
    case AttackId::NeutralSpecial: return "special_n";
    case AttackId::SideSpecial: return "special_s";
    case AttackId::UpSpecial: return "special_u";
    case AttackId::DownSpecial: return "special_d";
    case AttackId::None: case AttackId::Count: break;
    }
    return nullptr;
}

const data::MoveDefinition* find_move(const data::MoveCatalog& catalog, AttackId id) {
    const char* key = move_id_for_attack(id);
    if (key == nullptr) return nullptr;
    for (const data::MoveDefinition& move : catalog.moves) {
        if (move.id == key) return &move;
    }
    return nullptr;
}

// Midpoint of the AABB. Used for hit-spark placement.
Vec2 aabb_center(const Aabb& a) {
    return Vec2{ a.x + a.w * 0.5f, a.y + a.h * 0.5f };
}

// Per-match xorshift32 keeps presentation randomness deterministic and
// isolated: two simultaneous matches must not consume one shared process RNG.
std::uint32_t next_shake_random(std::uint32_t& state) {
    // xorshift32 has an absorbing zero state; keep the generator non-zero.
    if (state == 0) state = 0x6D2B79F5u;
    std::uint32_t value = state;
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    state = value;
    return value;
}

float shake_offset_1d(float magnitude, std::uint32_t& state) {
    const double unit = static_cast<double>(next_shake_random(state))
                      / static_cast<double>(std::numeric_limits<std::uint32_t>::max());
    return static_cast<float>((unit * 2.0 - 1.0) * magnitude);
}

} // namespace

Match::Match()
    : respawn_timer_{ -1, -1 } {
    data::ContentError content_error;
    std::string executable_directory;
if (const char* executable_base = SDL_GetBasePath()) {
    executable_directory = executable_base;
}

#ifdef EMBER_SOURCE_DIR
    const std::string source_directory = EMBER_SOURCE_DIR;
#else
    const std::string source_directory;
#endif
    const std::string catalog_path = data::resolve_move_catalog_path(
        {}, executable_directory, source_directory);
    move_catalog_loaded_ = data::load_move_catalog(catalog_path, move_catalog_, content_error);
    if (!move_catalog_loaded_) {
        Logger::warn("Move content unavailable; using legacy attack hitboxes: " + content_error.message);
    }
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
        respawn_timer_[i] = -1;
    }
    // AI controllers are not owned by the fighters; reset their internal
    // state but keep them attached so a rematch keeps the same difficulty.
    for (int i = 0; i < kFighterCount; ++i) {
        if (ai_[i]) ai_[i]->reset_for_test();
    }
    ko_banner_ticks_ = 0;
    phase_           = Phase::Fighting;
    last_hit_        = HitEvent{0.0f, 0.0f, false};
    hit_spark_age_   = 0;
    hit_stop_frames_ = 0;
    ko_shake_frames_ = 0;
    ko_shake_offset_x_ = 0.0f;
    ko_shake_offset_y_ = 0.0f;
    shake_rng_state_ = kDefaultShakeRngSeed;
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

void Match::set_ai(int fighter_index, std::nullptr_t) {
    set_ai(fighter_index, std::unique_ptr<AiController>{});
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

void Match::update(const InputManager& input) {
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
        ko_shake_offset_x_ = shake_offset_1d(mag, shake_rng_state_);
        ko_shake_offset_y_ = shake_offset_1d(mag, shake_rng_state_);
    } else {
        ko_shake_offset_x_ = 0.0f;
        ko_shake_offset_y_ = 0.0f;
    }

    // M5: when the match is over, freeze all fighter motion and just let
    // the KO banner timer tick. The rematch reset comes from App when the
    // player presses R.
    if (phase_ == Phase::MatchOver) {
        ++ko_banner_ticks_;
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
    // the previous frame does not freeze on screen. We also let the
    // KO shake tick down (a held shake from a prior match's
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
    //    gets the bookkeeping this frame instead of next. The KO tick is
    //    the timer's start boundary, not one of its 30 elapsed wait ticks.
    std::array<bool, kFighterCount> died_this_tick{};
    for (int i = 0; i < kFighterCount; ++i) {
        Fighter& f = *fighters_[i];
        if (!f.is_dead() && arena_.is_out_of_bounds(f.body())) {
            f.set_dead_for_test(true);
            died_this_tick[static_cast<std::size_t>(i)] = true;
            if (f.stocks() > 0) {
                f.set_stocks_for_test(f.stocks() - 1);
            }
            if (f.stocks() > 0) {
                respawn_timer_[i] = combat::kRespawnDelayTicks;
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
            ai_[i]->decide(self, opp, arena_,
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
        self.update(motion, combat, arena_);
    }

    // 3) Resolve data-authored active hitboxes. The immutable catalog is loaded
    // once at match construction; frame-local active shapes are derived from the
    // current fighter pose and tick, so rollback/replay needs no cached geometry.
    for (int a = 0; a < kFighterCount; ++a) {
        Fighter& attacker = *fighters_[a];
        if (attacker.state() != Fighter::State::Attacking || attacker.attack_has_hit()) continue;
        const data::MoveDefinition* move = move_catalog_loaded_
            ? find_move(move_catalog_, attacker.current_attack_id()) : nullptr;
        std::vector<data::ActiveHitbox> content_hitboxes;
        if (move != nullptr) {
            content_hitboxes = data::hitboxes_at_tick(
                *move, attacker.attack_frame(), attacker.body(), attacker.facing());
        } else {
            const Aabb legacy = attacker.active_hitbox();
            if (legacy.w > 0.0f && legacy.h > 0.0f) {
                data::ActiveHitbox fallback{};
                fallback.shape = data::HitboxShape::Aabb;
                fallback.bounds = legacy;
                const Attack& attack = get_attack(attacker.current_attack_id());
                fallback.damage = attack.damage;
                fallback.angle = static_cast<int>(std::lround(
                    std::atan2(-attack.knockback_angle_y, attack.knockback_angle_x)
                    * 180.0 / 3.14159265358979323846));
                fallback.base_knockback = attack.base_knockback;
                fallback.hitstun = attack.hitstun_frames;
                content_hitboxes.push_back(fallback);
            }
        }
        if (content_hitboxes.empty()) continue;

        for (int d = 0; d < kFighterCount; ++d) {
            if (d == a) continue;
            Fighter& target = *fighters_[d];
            if (target.is_dead()) continue;
            bool contact = false;
            data::ActiveHitbox contacted{};
            for (const data::ActiveHitbox& active : content_hitboxes) {
                if (data::hitbox_overlaps(active, target.body())) {
                    contact = true;
                    contacted = active;
                    break;
                }
            }
            if (!contact) continue;
            const Fighter::HitOutcome outcome = apply_attack(attacker, target, contacted);
            if (outcome != Fighter::HitOutcome::Ignored) {
                const Vec2 c = aabb_center(contacted.bounds);
                last_hit_ = HitEvent{static_cast<float>(c.x), static_cast<float>(c.y), true};
                hit_spark_age_ = combat::kHitSparkFrames;
                if (outcome == Fighter::HitOutcome::Hit)
                    hit_stop_frames_ = combat::kHitStopFrames;
            }
        }
    }

    // 4) Respawn tick: when a fighter's timer reaches 0, respawn them and
    //    apply the post-respawn invulnerability window. Skipped once the
    //    fighter's stocks have hit 0 - they stay off-stage and the match
    //    ends.
    for (int i = 0; i < kFighterCount; ++i) {
        if (died_this_tick[static_cast<std::size_t>(i)]) continue;
        if (respawn_timer_[i] < 0) continue;
        if (--respawn_timer_[i] <= 0) {
            respawn_timer_[i] = -1;
            fighters_[i]->respawn(arena_);
            fighters_[i]->set_respawn_invuln_ticks(
                combat::kRespawnInvulnTicks);
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
        ko_banner_ticks_ = 0;
        // M6-C: arm the post-KO shake. The actual offset is set
        // immediately to a fresh random value at full magnitude, then
        // ticked at the top of subsequent updates. We initialize the
        // offset here so the very first frame the renderer reads (this
        // same frame, via App::render) shows a meaningful shake rather
        // than the zero default.
        ko_shake_frames_ = combat::kKoShakeFrames;
        const float mag = combat::kKoShakeMagnitude;
        ko_shake_offset_x_ = shake_offset_1d(mag, shake_rng_state_);
        ko_shake_offset_y_ = shake_offset_1d(mag, shake_rng_state_);
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
// M11: simulation-state fingerprint. This folds exact floating-point bits,
// every fighter resource/timer that can affect later ticks, AI runtime and
// tuning state, training playback, and match-flow timers. It is a desync
// diagnostic for identical builds/configurations; it does not promise
// cross-platform floating-point determinism.
// --------------------------------------------------------------------
Match::Snapshot Match::capture_state() const {
    Snapshot snapshot{};
    snapshot.version = 2;
    snapshot.move_content_hash = move_catalog_loaded_ ? move_catalog_.content_hash : 0;
    snapshot.arena_id = arena_id_;
    for (int i = 0; i < kFighterCount; ++i) {
        snapshot.fighters[static_cast<std::size_t>(i)] = fighters_[i]->capture_state();
        snapshot.has_ai[static_cast<std::size_t>(i)] = ai_[i] != nullptr;
        if (ai_[i]) {
            snapshot.ai[static_cast<std::size_t>(i)] = ai_[i]->capture();
        }
    }
    snapshot.training = training_recorder_.capture();
    snapshot.respawn_timer = respawn_timer_;
    snapshot.ko_banner_ticks = ko_banner_ticks_;
    snapshot.phase = phase_;
    snapshot.last_hit_x = last_hit_.x;
    snapshot.last_hit_y = last_hit_.y;
    snapshot.last_hit_valid = last_hit_.valid;
    snapshot.hit_spark_age = hit_spark_age_;
    snapshot.match_startup_ticks = match_startup_ticks_;
    snapshot.intro_banner_state = intro_banner_state_;
    snapshot.live_ticks = live_ticks_;
    snapshot.hit_stop_frames = hit_stop_frames_;
    snapshot.ko_shake_frames = ko_shake_frames_;
    snapshot.ko_shake_offset_x = ko_shake_offset_x_;
    snapshot.ko_shake_offset_y = ko_shake_offset_y_;
    snapshot.shake_rng_state = shake_rng_state_;
    return snapshot;
}

bool Match::restore_state(const Snapshot& snapshot) {
    if (snapshot.version != 2
        || snapshot.move_content_hash != (move_catalog_loaded_ ? move_catalog_.content_hash : 0)
        || static_cast<int>(snapshot.arena_id) < static_cast<int>(Arena::Id::TrainingRing)
        || static_cast<int>(snapshot.arena_id) > static_cast<int>(Arena::Id::SkyTemple)
        || static_cast<int>(snapshot.phase) < static_cast<int>(Phase::Fighting)
        || static_cast<int>(snapshot.phase) > static_cast<int>(Phase::MatchOver)
        || !std::isfinite(snapshot.last_hit_x) || !std::isfinite(snapshot.last_hit_y)
        || !std::isfinite(snapshot.ko_shake_offset_x)
        || !std::isfinite(snapshot.ko_shake_offset_y)
        || snapshot.ko_banner_ticks < 0 || snapshot.hit_spark_age < 0
        || snapshot.match_startup_ticks < 0
        || snapshot.intro_banner_state < 0 || snapshot.intro_banner_state > 2
        || snapshot.live_ticks < 0 || snapshot.hit_stop_frames < 0
        || snapshot.ko_shake_frames < 0) {
        return false;
    }
    for (int timer : snapshot.respawn_timer) {
        if (timer < -1) return false;
    }

    // Validate and build all replacement objects before changing the live
    // match. An invalid snapshot therefore leaves the current simulation intact.
    Arena candidate_arena;
    candidate_arena.reset(snapshot.arena_id);

    std::array<std::unique_ptr<Fighter>, kFighterCount> candidate_fighters{};
    std::array<std::unique_ptr<AiController>, kFighterCount> candidate_ai{};
    for (int i = 0; i < kFighterCount; ++i) {
        candidate_fighters[static_cast<std::size_t>(i)] =
            std::make_unique<Fighter>(i, candidate_arena);
        if (!candidate_fighters[static_cast<std::size_t>(i)]->restore_state(
                snapshot.fighters[static_cast<std::size_t>(i)])) {
            return false;
        }
        if (snapshot.has_ai[static_cast<std::size_t>(i)]) {
            const auto& ai_snapshot = snapshot.ai[static_cast<std::size_t>(i)];
            auto controller = std::make_unique<AiController>(ai_snapshot.difficulty);
            if (!controller->restore(ai_snapshot)) return false;
            candidate_ai[static_cast<std::size_t>(i)] = std::move(controller);
        }
    }
    training::Recorder candidate_training;
    if (!candidate_training.restore(snapshot.training)) return false;

    arena_ = candidate_arena;
    arena_id_ = snapshot.arena_id;
    fighters_ = std::move(candidate_fighters);
    ai_ = std::move(candidate_ai);
    training_recorder_ = candidate_training;
    respawn_timer_ = snapshot.respawn_timer;
    ko_banner_ticks_ = snapshot.ko_banner_ticks;
    phase_ = snapshot.phase;
    last_hit_ = HitEvent{snapshot.last_hit_x, snapshot.last_hit_y,
                         snapshot.last_hit_valid};
    hit_spark_age_ = snapshot.hit_spark_age;
    match_startup_ticks_ = snapshot.match_startup_ticks;
    intro_banner_state_ = snapshot.intro_banner_state;
    live_ticks_ = snapshot.live_ticks;
    hit_stop_frames_ = snapshot.hit_stop_frames;
    ko_shake_frames_ = snapshot.ko_shake_frames;
    ko_shake_offset_x_ = snapshot.ko_shake_offset_x;
    ko_shake_offset_y_ = snapshot.ko_shake_offset_y;
    shake_rng_state_ = snapshot.shake_rng_state;
    return true;
}

std::uint64_t Match::state_hash() const {
    static_assert(sizeof(float) == sizeof(std::uint32_t));
    static_assert(sizeof(double) == sizeof(std::uint64_t));
    static_assert(std::numeric_limits<float>::is_iec559);
    static_assert(std::numeric_limits<double>::is_iec559);

    // FNV-1a 64-bit. Floating-point values are folded by their exact IEEE-754
    // representation; numeric casts would erase fractional movement and can
    // hide a desync until it becomes visually obvious.
    constexpr std::uint64_t kFnvOffset = 0xcbf29ce484222325ULL;
    constexpr std::uint64_t kFnvPrime  = 0x100000001b3ULL;
    std::uint64_t h = kFnvOffset;

    auto mix32 = [&](std::uint32_t value) {
        for (int byte = 0; byte < 4; ++byte) {
            h ^= (value >> (byte * 8)) & 0xFFu;
            h *= kFnvPrime;
        }
    };
    auto mix64 = [&](std::uint64_t value) {
        mix32(static_cast<std::uint32_t>(value));
        mix32(static_cast<std::uint32_t>(value >> 32));
    };
    auto mix_f32 = [&](float value) {
        mix32(std::bit_cast<std::uint32_t>(value));
    };
    auto mix_f64 = [&](double value) {
        mix64(std::bit_cast<std::uint64_t>(value));
    };
    auto mix_bool = [&](bool value) { mix32(value ? 1u : 0u); };

    // Content identity is simulation-relevant: matching frame state with a
    // different move catalog must not be treated as a replay-equivalent state.
    mix64(move_catalog_loaded_ ? move_catalog_.content_hash : 0);

    // --- Arena data ---
    mix32(static_cast<std::uint32_t>(arena_.id()));
    for (const Platform& platform : arena_.platforms()) {
        mix_f32(platform.aabb.x);
        mix_f32(platform.aabb.y);
        mix_f32(platform.aabb.w);
        mix_f32(platform.aabb.h);
        mix_bool(platform.one_way);
        mix_bool(platform.ledge_grabbable);
    }
    mix_f32(arena_.blast_zone().left);
    mix_f32(arena_.blast_zone().right);
    mix_f32(arena_.blast_zone().top);
    mix_f32(arena_.blast_zone().bottom);
    mix_f32(arena_.spawn_x(0));
    mix_f32(arena_.spawn_x(1));
    mix_f32(arena_.spawn_y());

    // --- All fighter state that can affect a later simulation tick ---
    for (int i = 0; i < kFighterCount; ++i) {
        const Fighter& f = *fighters_[i];
        const Aabb& body = f.body();
        mix_f32(body.x);
        mix_f32(body.y);
        mix_f32(body.w);
        mix_f32(body.h);
        mix_f64(f.velocity().x);
        mix_f64(f.velocity().y);
        mix_bool(f.is_grounded());
        mix_bool(f.is_dead());
        mix32(static_cast<std::uint32_t>(f.player_index()));
        mix32(static_cast<std::uint32_t>(f.damage_percent()));
        mix32(static_cast<std::uint32_t>(f.stocks()));
        mix32(static_cast<std::uint32_t>(f.facing()));
        mix32(static_cast<std::uint32_t>(f.jumps_remaining()));
        mix32(static_cast<std::uint32_t>(f.jump_buffer_ticks()));
        mix32(static_cast<std::uint32_t>(f.coyote_ticks()));
        mix32(static_cast<std::uint32_t>(f.state()));
        mix32(static_cast<std::uint32_t>(f.current_attack_id()));
        mix32(static_cast<std::uint32_t>(f.attack_frame()));
        mix_bool(f.attack_has_hit());
        mix32(static_cast<std::uint32_t>(f.hitstun_frames()));
        mix32(static_cast<std::uint32_t>(f.invuln_frames()));
        mix32(static_cast<std::uint32_t>(f.shield()));
        mix32(static_cast<std::uint32_t>(f.guard_frames()));
        mix32(static_cast<std::uint32_t>(f.dodge_frame()));
        mix32(static_cast<std::uint32_t>(f.dodge_cooldown()));
        mix32(static_cast<std::uint32_t>(f.dodge_direction()));
        mix32(static_cast<std::uint32_t>(f.surge_meter()));
        mix32(static_cast<std::uint32_t>(f.surge_frames()));
        mix32(static_cast<std::uint32_t>(f.parry_flash()));
        mix_bool(f.up_special_used());
        mix_bool(f.air_dodge_used());
        mix32(static_cast<std::uint32_t>(f.tech_buffer_ticks()));
        mix32(static_cast<std::uint32_t>(f.tech_frames()));
        mix32(static_cast<std::uint32_t>(f.ledge_lockout_ticks()));
        mix32(static_cast<std::uint32_t>(f.ledge_hang_ticks()));
        const Fighter::Snapshot fighter_state = f.capture_state();
        mix_f64(fighter_state.di_reference_velocity.x);
        mix_f64(fighter_state.di_reference_velocity.y);
        mix_bool(fighter_state.ledge_invulnerability_used);
        mix_f32(fighter_state.ledge_climb_x);
        mix_f32(fighter_state.ledge_platform_top);
        // Animation timing affects the next visible frame and is included so
        // snapshots that restore a different animation cursor cannot hash as
        // equivalent simulation checkpoints.
        mix32(static_cast<std::uint32_t>(fighter_state.animation.pose));
        mix32(static_cast<std::uint32_t>(fighter_state.animation.frame_in_pose));
        mix32(static_cast<std::uint32_t>(fighter_state.animation.ticks_until_next));
        mix32(static_cast<std::uint32_t>(fighter_state.animation.timing.idle_ticks_per_frame));
        mix32(static_cast<std::uint32_t>(fighter_state.animation.timing.walk_ticks_per_frame));
        mix32(static_cast<std::uint32_t>(fighter_state.animation.timing.jump_ticks_per_frame));
        mix32(static_cast<std::uint32_t>(fighter_state.animation.timing.attack_ticks_per_frame));
        mix32(static_cast<std::uint32_t>(fighter_state.animation.timing.hitstun_ticks_per_frame));
    }

    // --- AI controller state and tuning ---
    for (int i = 0; i < kFighterCount; ++i) {
        const AiController* controller = ai_[i].get();
        mix_bool(controller != nullptr);
        if (!controller) continue;

        mix32(static_cast<std::uint32_t>(controller->state()));
        mix32(static_cast<std::uint32_t>(controller->state_ticks()));
        mix32(static_cast<std::uint32_t>(controller->last_attack_tick()));
        mix32(controller->rng_state());
        mix_bool(controller->skip_grace_for_test());

        const ai::Difficulty& difficulty = controller->difficulty();
        mix32(static_cast<std::uint32_t>(difficulty.reaction_delay_ticks));
        mix32(static_cast<std::uint32_t>(difficulty.attack_frequency));
        mix32(static_cast<std::uint32_t>(difficulty.recovery_priority));
        mix32(static_cast<std::uint32_t>(difficulty.defensive_threshold_percent));
        mix32(static_cast<std::uint32_t>(difficulty.jump_aggression));
        mix32(difficulty.seed);
    }

    // --- Training recording / playback state ---
    mix32(static_cast<std::uint32_t>(training_recorder_.size()));
    mix32(static_cast<std::uint32_t>(training_recorder_.cursor()));
    mix_bool(training_recorder_.recording());
    mix_bool(training_recorder_.playing());
    mix_bool(training_recorder_.overflowed());
    for (std::size_t i = 0; i < training_recorder_.size(); ++i) {
        const training::RecordedFrame* frame = training_recorder_.frame_at(i);
        if (!frame) {
            mix32(0xFFFFFFFFu);
            continue;
        }
        const PlayerIntent& motion = frame->motion;
        const CombatIntent& combat = frame->combat;
        mix_bool(motion.move_left);
        mix_bool(motion.move_right);
        mix_bool(motion.jump_pressed);
        mix_bool(motion.fast_fall_held);
        mix_bool(motion.up_held);
        mix_bool(motion.run_held);
        mix_bool(combat.jab_pressed);
        mix_bool(combat.ftilt_pressed);
        mix_bool(combat.nair_pressed);
        mix_bool(combat.special_pressed);
        mix_bool(combat.smash_held);
        mix_bool(combat.shield_held);
        mix_bool(combat.shield_pressed);
        mix_bool(combat.dodge_pressed);
        mix_bool(combat.surge_pressed);
    }

    // --- Match flow, timers, hit feedback and simulation-age counters ---
    mix32(static_cast<std::uint32_t>(phase_));
    mix32(static_cast<std::uint32_t>(ko_banner_ticks_));
    mix32(static_cast<std::uint32_t>(match_startup_ticks_));
    mix32(static_cast<std::uint32_t>(intro_banner_state_));
    mix32(static_cast<std::uint32_t>(live_ticks_));
    for (int timer : respawn_timer_) mix32(static_cast<std::uint32_t>(timer));
    mix32(static_cast<std::uint32_t>(hit_spark_age_));
    mix_f32(last_hit_.x);
    mix_f32(last_hit_.y);
    mix_bool(last_hit_.valid);
    mix32(static_cast<std::uint32_t>(hit_stop_frames_));
    mix32(static_cast<std::uint32_t>(ko_shake_frames_));
    mix32(shake_rng_state_);
    mix_f32(ko_shake_offset_x_);
    mix_f32(ko_shake_offset_y_);

    return h;
}

} // namespace ember
