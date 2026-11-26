#include "Gameplay/Fighter.h"

#include "Gameplay/CombatConfig.h"
#include "Graphics/Renderer.h"
#include "Physics/PhysicsConfig.h"
#include "World/Arena.h"

#include <algorithm>
#include <cmath>

namespace ember {

namespace {
// Apply one tick of directional influence to a launch vector. DI may rotate
// the current trajectory by at most the configured angle from the original
// knockback direction, without changing its speed. It cannot increase the
// pre-DI downward component; an upward/horizontal launch cannot be steered
// into a downward trajectory. Gravity is applied separately afterwards.
Vec2 apply_directional_influence(Vec2 velocity, Vec2 launch,
                                  double input_x, double input_y) {
    constexpr double kPi = 3.14159265358979323846;
    const double current_speed = std::hypot(velocity.x, velocity.y);
    if (current_speed <= 1e-9 || (input_x == 0.0 && input_y == 0.0))
        return velocity;

    Vec2 candidate{
        velocity.x + input_x * physics::kDirectionalInfluenceAcceleration,
        velocity.y + input_y * physics::kDirectionalInfluenceAcceleration,
    };
    const double candidate_speed = std::hypot(candidate.x, candidate.y);
    if (candidate_speed <= 1e-9) return velocity;

    const double launch_angle = std::atan2(launch.y, launch.x);
    const double candidate_angle = std::atan2(candidate.y, candidate.x);
    const double delta = std::remainder(candidate_angle - launch_angle, 2.0 * kPi);
    const double max_rotation = physics::kDirectionalInfluenceMaxRotationDegrees
                              * (kPi / 180.0);
    const double clamped_angle = launch_angle
        + std::clamp(delta, -max_rotation, max_rotation);

    candidate.x = current_speed * std::cos(clamped_angle);
    candidate.y = current_speed * std::sin(clamped_angle);

    // DI may make a downward launch less downward, but must never exceed the
    // original launch's downward component. An upward launch cannot be turned
    // into a downward trajectory. Gravity is applied separately afterwards.
    const double max_downward_component = launch.y > 0.0
        ? velocity.y : 0.0;
    if (candidate.y > max_downward_component) {
        candidate.y = max_downward_component;
        const double horizontal = std::sqrt(std::max(
            0.0, current_speed * current_speed
                 - candidate.y * candidate.y));
        const double direction = candidate.x != 0.0 ? candidate.x : launch.x;
        candidate.x = std::copysign(horizontal, direction);
    }
    return candidate;
}

// Apply gravity (and fast-fall, when held mid-air) to v without touching
// horizontal velocity. Shared by Attacking and Hitstun motion paths.
Vec2 apply_gravity_only(Vec2 v, bool fast_fall_held) {
    v.y += physics::kGravity;
    if (v.y > physics::kTerminalVelocity) {
        v.y = physics::kTerminalVelocity;
    }
    if (fast_fall_held && v.y > 0.0) {
        const double extra = physics::kGravity * (physics::kFastFallMultiplier - 1.0);
        v.y += extra;
        if (v.y > physics::kTerminalVelocity) {
            v.y = physics::kTerminalVelocity;
        }
    }
    return v;
}
} // namespace

Fighter::Fighter(int player_index, const Arena& arena)
    : player_index_(player_index),
      sprite_(std::make_unique<Sprite>(Sprite::build_animation_strip(
          art::palette_for(player_index, art::color_blind_enabled())))) {
    respawn(arena);
}

void Fighter::respawn(const Arena& arena) {
    body_.x = arena.spawn_x(player_index_);
    body_.y = arena.spawn_y() - kHeight;  // feet flush with the spawn y
    body_.w = kWidth;
    body_.h = kHeight;
    velocity_       = {0.0, 0.0};
    di_reference_velocity_ = {0.0, 0.0};
    grounded_       = true;
    jumps_remaining_ = 2;
    dead_           = false;
    damage_percent_ = 0;
    facing_         = (player_index_ == 0) ? 1 : -1;
    state_          = State::Idle;
    current_attack_id_ = AttackId::None;
    attack_frame_      = 0;
    attack_has_hit_    = false;
    hitstun_frames_    = 0;
    invuln_frames_     = 0;
    shield_ = combat::kShieldMax; guard_frames_ = 0; dodge_frame_ = 0;
    dodge_cooldown_ = 0; surge_frames_ = 0; parry_flash_ = 0;
    up_special_used_ = false; air_dodge_used_ = false;
    tech_buffer_ticks_ = 0; tech_frames_ = 0;
    ledge_lockout_ticks_ = 0; ledge_hang_ticks_ = 0;
    ledge_invulnerability_used_ = false;
    ledge_climb_x_ = 0.0f; ledge_platform_top_ = 0.0f;

    // First call (from the constructor) initializes stocks to the starting
    // value; subsequent calls (from Match on respawn) preserve the count so
    // we don't refill lives mid-match.
    if (stocks_ == 0) {
        stocks_ = combat::kStartingStocks;
    }

    // M6-B: a respawned fighter must not show the previous death's last
    // pose for one tick. Reset the animation driver to Idle and pin the
    // sprite to frame 0 so the first frame after respawn is the clean
    // idle silhouette.
    anim_driver_.reset_to_idle();
    if (sprite_) sprite_->set_current_frame(anim_driver_.global_frame());
}

// Called by Match immediately after a respawn. Durations are already expressed
// in fixed simulation ticks, so no seconds-to-frames conversion is required.
void Fighter::set_respawn_invuln_ticks(int ticks) {
    invuln_frames_ = ticks > 0 ? ticks : 0;
}

// Used by Match::reset() to wipe a rematch's per-fighter state back to a
// fresh match. Resets stocks, damage, and any in-progress attack/hitstun.
void Fighter::reset_for_rematch() {
    stocks_        = combat::kStartingStocks;
    surge_meter_   = 0;
    shield_ = combat::kShieldMax; guard_frames_ = 0; dodge_frame_ = 0;
    dodge_cooldown_ = 0; surge_frames_ = 0; parry_flash_ = 0;
    up_special_used_ = false; air_dodge_used_ = false;
    tech_buffer_ticks_ = 0; tech_frames_ = 0;
    ledge_lockout_ticks_ = 0; ledge_hang_ticks_ = 0;
    ledge_invulnerability_used_ = false;
    ledge_climb_x_ = 0.0f; ledge_platform_top_ = 0.0f;
    di_reference_velocity_ = Vec2{0.0, 0.0};
    damage_percent_ = 0;
    dead_           = false;
    invuln_frames_  = 0;
    hitstun_frames_ = 0;
    state_          = State::Idle;
    current_attack_id_ = AttackId::None;
    attack_frame_      = 0;
    attack_has_hit_    = false;
}

// P15: rebuild the per-fighter sprite from the current color-blind
// flag. The renderer's sprite cache is keyed on Sprite* identity, so
// we allocate a fresh Sprite (with a fresh address) to ensure the
// cache slot's old texture is dropped and the new palette is uploaded
// on the next render. We also reset the animation driver to Idle
// and pin the sprite to frame 0 so the fighter doesn't show a
// half-faded frame for a tick.
void Fighter::rebuild_sprite_for_palette() {
    sprite_ = std::make_unique<Sprite>(Sprite::build_animation_strip(
        art::palette_for(player_index_, art::color_blind_enabled())));
    if (sprite_) {
        sprite_->set_current_frame(0);
    }
    anim_driver_.reset_to_idle();
}

bool Fighter::try_start_attack(const PlayerIntent& mi, const CombatIntent& ci) {
    // Directions relative to facing.
    const bool right = mi.move_right && !mi.move_left;
    const bool left  = mi.move_left  && !mi.move_right;
    const bool fwd   = (facing_ > 0) ? right : left;
    const bool back  = (facing_ > 0) ? left  : right;
    const bool up    = mi.up_held && !mi.fast_fall_held;
    const bool down  = mi.fast_fall_held && !mi.up_held;

    // Input resolution. Priority: special > jab > strong > air button, which
    // keeps the legacy priority (jab beat ftilt beat nair) intact.
    AttackId requested = AttackId::None;
    auto aerial = [&]() {
        if (up)   return AttackId::UpAir;
        if (down) return AttackId::DownAir;
        if (fwd)  return AttackId::ForwardAir;
        if (back) return AttackId::BackAir;
        return AttackId::NeutralAir;
    };
    if (ci.nair_pressed)  requested = grounded_ ? AttackId::NeutralAir : aerial();
    if (ci.ftilt_pressed) {
        if (!grounded_)            requested = aerial();   // strong button in air = aerial
        else if (ci.smash_held)    requested = up ? AttackId::UpSmash
                                          : down ? AttackId::DownSmash
                                                 : AttackId::ForwardSmash;
        else                       requested = up ? AttackId::UpTilt
                                          : down ? AttackId::DownTilt
                                                 : AttackId::ForwardTilt;
    }
    if (ci.jab_pressed) {
        if (!grounded_)  requested = aerial();
        else             requested = up ? AttackId::UpTilt
                                  : down ? AttackId::DownTilt : AttackId::Jab;
    }
    if (ci.special_pressed) {
        if (up)                requested = AttackId::UpSpecial;
        else if (down)         requested = AttackId::DownSpecial;
        else if (fwd || back)  requested = AttackId::SideSpecial;
        else                   requested = AttackId::NeutralSpecial;
    }
    if (requested == AttackId::None) return false;

    const Attack& atk = get_attack(requested);
    if (atk.requires_ground && !grounded_) return false;
    if (atk.requires_air    &&  grounded_) return false;
    if (atk.once_per_airtime && up_special_used_) return false;

    if (requested == AttackId::SideSpecial && back) facing_ = -facing_;

    current_attack_id_ = requested;
    attack_frame_      = 0;
    attack_has_hit_    = false;
    state_             = State::Attacking;
    if (atk.self_vx != 0.0f) velocity_.x = static_cast<double>(atk.self_vx) * facing_;
    if (atk.self_vy != 0.0f) {
        velocity_.y = static_cast<double>(atk.self_vy);
        if (atk.self_vy < 0.0f) grounded_ = false;
    }
    if (atk.once_per_airtime) up_special_used_ = true;
    return true;
}

void Fighter::add_surge(int amount) {
    if (surge_frames_ > 0 || amount <= 0) return;   // no gain during Surge
    surge_meter_ += amount;
    if (surge_meter_ > combat::kSurgeMeterMax) surge_meter_ = combat::kSurgeMeterMax;
}

void Fighter::stun_for(int frames) {
    if (state_ == State::LedgeHang) {
        ledge_lockout_ticks_ = physics::kLedgeRegrabLockoutTicks;
        ledge_hang_ticks_ = 0;
        grounded_ = false;
    }
    tech_buffer_ticks_ = 0;
    tech_frames_ = 0;
    di_reference_velocity_ = velocity_;
    hitstun_frames_ = frames < combat::kMinHitstunFrames ? combat::kMinHitstunFrames : frames;
    state_ = State::Hitstun;
    current_attack_id_ = AttackId::None;
    attack_frame_ = 0;
    attack_has_hit_ = false;
    guard_frames_ = 0;
}

Fighter::HitOutcome Fighter::receive_hit(int damage, Vec2 kb, int hitstun, int attacker_facing) {
    if (dead_ || invuln_frames_ > 0) return HitOutcome::Ignored;
    if (dodge_invulnerable()) {
        add_surge(combat::kSurgeGainDodge);
        return HitOutcome::Dodged;
    }
    if (state_ == State::Guard) {
        if (guard_frames_ <= combat::kParryWindowFrames) {
            parry_flash_ = 12;
            add_surge(combat::kSurgeGainParry);
            return HitOutcome::Parried;
        }
        shield_ -= damage * combat::kShieldDamageCost;
        velocity_.x = combat::kGuardPushback * attacker_facing;
        add_surge(combat::kSurgeGainBlock);
        if (shield_ <= 0) {
            shield_ = combat::kShieldBreakRestore;
            stun_for(combat::kShieldBreakStun);
        }
        return HitOutcome::Blocked;
    }
    apply_hit(damage, kb, hitstun);
    add_surge(damage * combat::kSurgeGainPerDamageTaken);
    return HitOutcome::Hit;
}

void Fighter::advance_attack() {
    ++attack_frame_;
    const Attack& atk = get_attack(current_attack_id_);
    if (attack_frame_ >= attack_total_frames(atk)) {
        state_             = State::Idle;
        current_attack_id_ = AttackId::None;
        attack_frame_      = 0;
        attack_has_hit_    = false;
    }
}

void Fighter::update(const PlayerIntent& motion_intent,
                     const CombatIntent& combat_intent,
                     const Arena& arena) {
    if (dead_) return;

    // --- Tick-based defensive windows -------------------------------------
    if (invuln_frames_ > 0) --invuln_frames_;
    if (dodge_cooldown_ > 0) --dodge_cooldown_;
    if (parry_flash_ > 0) --parry_flash_;
    if (surge_frames_ > 0) --surge_frames_;
    if (ledge_lockout_ticks_ > 0) --ledge_lockout_ticks_;

    const bool started_tick_in_air_hitstun = state_ == State::Hitstun && !grounded_;
    if (started_tick_in_air_hitstun && combat_intent.shield_pressed) {
        tech_buffer_ticks_ = physics::kTechInputBufferTicks;
    } else if (tech_buffer_ticks_ > 0) {
        --tech_buffer_ticks_;
    }

    // --- State advance -----------------------------------------------------
    if (state_ == State::Hitstun) {
        if (--hitstun_frames_ <= 0) {
            hitstun_frames_ = 0;
            state_ = State::Idle;
            di_reference_velocity_ = Vec2{0.0, 0.0};
        }
    }
    if (state_ == State::Attacking) advance_attack();
    if (state_ == State::Dodge) {
        if (++dodge_frame_ >= combat::kDodgeFrames) {
            dodge_frame_ = 0;
            state_ = State::Idle;
        }
    }
    if (state_ == State::Tech) {
        if (++tech_frames_ >= physics::kTechDurationTicks) {
            tech_frames_ = 0;
            state_ = State::Idle;
        }
    }
    if (state_ == State::LedgeHang) {
        ++ledge_hang_ticks_;
    }
    if (state_ == State::Guard) {
        ++guard_frames_;
        shield_ -= combat::kShieldDrainPerFrame;
        if (!combat_intent.shield_held || !grounded_ || shield_ <= 0) {
            if (shield_ <= 0) {
                shield_ = combat::kShieldBreakRestore;
                stun_for(combat::kShieldBreakStun);
            } else {
                state_ = State::Idle;
            }
            guard_frames_ = 0;
        }
    } else if (shield_ < combat::kShieldMax) {
        shield_ += combat::kShieldRegenPerFrame;
        if (shield_ > combat::kShieldMax) shield_ = combat::kShieldMax;
    }

    // --- Ledge actions -----------------------------------------------------
    bool ledge_action_taken = false;
    bool ledge_climbed = false;
    // Ledge hang freezes physics. Up climbs onto the platform, jump launches
    // inward, and steering away (or timeout) drops the fighter cleanly.
    if (state_ == State::LedgeHang) {
        const bool jump = motion_intent.jump_pressed;
        const bool climb = motion_intent.up_held;
        const bool drop_away = (facing_ > 0 && motion_intent.move_left
                                && !motion_intent.move_right)
                            || (facing_ < 0 && motion_intent.move_right
                                && !motion_intent.move_left);
        if (jump) {
            ledge_action_taken = true;
            state_ = State::Idle;
            velocity_.x = physics::kLedgeJumpHorizontalSpeed * facing_;
            velocity_.y = physics::kLedgeJumpVelocity;
            grounded_ = false;
            jumps_remaining_ = 1;
            jump_buffer_ticks_ = 0;
            coyote_ticks_ = 0;
            ledge_lockout_ticks_ = physics::kLedgeRegrabLockoutTicks;
            ledge_hang_ticks_ = 0;
        } else if (climb) {
            ledge_action_taken = true;
            ledge_climbed = true;
            state_ = State::Idle;
            body_.x = ledge_climb_x_;
            body_.y = ledge_platform_top_ - body_.h;
            velocity_ = Vec2{0.0, 0.0};
            grounded_ = true;
            jumps_remaining_ = 2;
            jump_buffer_ticks_ = 0;
            coyote_ticks_ = 0;
            ledge_hang_ticks_ = 0;
        } else if (drop_away || ledge_hang_ticks_ >= physics::kLedgeHangMaxTicks) {
            ledge_action_taken = true;
            state_ = State::Idle;
            velocity_.x = -static_cast<double>(facing_) * 1.5;
            velocity_.y = 2.5;
            grounded_ = false;
            ledge_lockout_ticks_ = physics::kLedgeRegrabLockoutTicks;
            ledge_hang_ticks_ = 0;
        } else {
            velocity_ = Vec2{0.0, 0.0};
            grounded_ = false;
        }
    }

    // --- Ember Surge activation --------------------------------------------
    if (combat_intent.surge_pressed && surge_frames_ == 0
        && surge_meter_ >= combat::kSurgeMeterMax
        && state_ != State::Hitstun && state_ != State::LedgeHang) {
        surge_meter_ = 0;
        surge_frames_ = combat::kSurgeFrames;
    }

    // --- Dodge (from Idle or Guard) ---------------------------------------
    if ((state_ == State::Idle || state_ == State::Guard)
        && combat_intent.dodge_pressed && dodge_cooldown_ == 0
        && (grounded_ || !air_dodge_used_)) {
        state_ = State::Dodge;
        dodge_frame_ = 0;
        guard_frames_ = 0;
        dodge_cooldown_ = combat::kDodgeCooldownFrames;
        dodge_dir_ = motion_intent.move_right && !motion_intent.move_left ? 1
                   : motion_intent.move_left && !motion_intent.move_right ? -1 : 0;
        if (!grounded_) air_dodge_used_ = true;
    }

    // --- Try to start a new attack / guard from Idle -----------------------
    if (state_ == State::Idle) {
        if (!try_start_attack(motion_intent, combat_intent)
            && combat_intent.shield_held && grounded_ && shield_ > 0) {
            state_ = State::Guard;
            guard_frames_ = 1;
        }
    }

    if (state_ == State::Idle) {
        if (motion_intent.move_left && !motion_intent.move_right) facing_ = -1;
        if (motion_intent.move_right && !motion_intent.move_left) facing_ = 1;
    }

    // --- Motion ------------------------------------------------------------
    const bool is_hanging = state_ == State::LedgeHang;
    if (is_hanging || ledge_climbed) {
        // No integration or collision while attached to a ledge or on the
        // exact tick the climb transition places the fighter on the platform.
    } else if (ledge_action_taken) {
        // A ledge jump/drop is already a fully-resolved movement decision;
        // do not feed its jump edge back into the normal jump-buffer system.
        velocity_ = apply_gravity_only(velocity_, false);
    } else if (state_ == State::Idle) {
        const bool drop_through = motion_intent.fast_fall_held
            && motion_intent.jump_pressed && grounded_
            && arena.supported_by_one_way(body_);
        PlayerIntent effective_intent = motion_intent;
        if (drop_through) effective_intent.jump_pressed = false;
        const MotionState in{velocity_, grounded_, jumps_remaining_,
                             jump_buffer_ticks_, coyote_ticks_};
        const MotionResult out = step_motion(in, effective_intent);
        velocity_ = out.velocity;
        jumps_remaining_ = out.jumps_remaining;
        jump_buffer_ticks_ = out.jump_buffer_ticks;
        coyote_ticks_ = out.coyote_ticks;
    } else if (state_ == State::Tech) {
        velocity_.x *= 0.62;
        if (velocity_.x > -0.08 && velocity_.x < 0.08) velocity_.x = 0.0;
        velocity_.y = 0.0;
    } else if (state_ == State::Guard) {
        velocity_.x *= 0.5;
        velocity_ = apply_gravity_only(velocity_, false);
    } else if (state_ == State::Dodge) {
        velocity_.x = combat::kDodgeSpeed * dodge_dir_;
        if (grounded_) velocity_ = apply_gravity_only(velocity_, false);
        else velocity_.y = 0.0;
    } else {
        // Directional influence is applied only in airborne hitstun. It is
        // clamped to the original knockback angle and cannot steer further
        // downward into the stage.
        if (state_ == State::Hitstun && !grounded_) {
            const double dx = motion_intent.move_right == motion_intent.move_left
                ? 0.0 : (motion_intent.move_right ? 1.0 : -1.0);
            const double dy = motion_intent.up_held == motion_intent.fast_fall_held
                ? 0.0 : (motion_intent.up_held ? -1.0 : 1.0);
            velocity_ = apply_directional_influence(
                velocity_, di_reference_velocity_, dx, dy);
        }
        velocity_ = apply_gravity_only(velocity_, motion_intent.fast_fall_held);
    }

    // --- Integrate and resolve collision -----------------------------------
    if (!is_hanging && !ledge_climbed) {
        const bool drop_through = state_ == State::Idle
            && motion_intent.fast_fall_held && motion_intent.jump_pressed
            && arena.supported_by_one_way(body_);
        const bool was_grounded = grounded_;
        if (state_ == State::Tech) {
            // Tech movement is a grounded horizontal roll. Probe one pixel
            // below the feet afterwards so walking off the platform ends the
            // grounded recovery instead of levitating at its edge.
            arena.move_and_resolve(body_, velocity_);
            Aabb support_probe = body_;
            support_probe.y += 1.0f;
            Vec2 probe_velocity{0.0, 1.0};
            grounded_ = arena.resolve_collision(support_probe, probe_velocity);
            if (!grounded_) state_ = State::Idle;
        } else {
            grounded_ = arena.move_and_resolve(body_, velocity_, drop_through);
        }

        // A fresh shield press buffered during airborne hitstun becomes a
        // tech when the fighter lands within the eight-tick input window.
        if (!was_grounded && grounded_ && tech_buffer_ticks_ > 0
            && (state_ == State::Hitstun || state_ == State::Idle)) {
            state_ = State::Tech;
            hitstun_frames_ = 0;
            tech_buffer_ticks_ = 0;
            tech_frames_ = 0;
            velocity_.y = 0.0;
            if (motion_intent.move_left != motion_intent.move_right) {
                velocity_.x = motion_intent.move_right ? 4.5 : -4.5;
            } else {
                velocity_.x *= 0.35;
            }
        }

        // Falling near a platform edge can be caught. The arena computes the
        // deterministic hang and climb anchors; Fighter owns the state change.
        if (!grounded_ && ledge_lockout_ticks_ == 0
            && state_ != State::Tech && state_ != State::LedgeHang) {
            const LedgeGrab ledge = arena.find_ledge_grab(body_, velocity_, facing_);
            if (ledge.found) {
                body_.x = ledge.hang_x;
                body_.y = ledge.hang_y;
                velocity_ = Vec2{0.0, 0.0};
                facing_ = ledge.facing_inward;
                state_ = State::LedgeHang;
                current_attack_id_ = AttackId::None;
                attack_frame_ = 0;
                attack_has_hit_ = false;
                ledge_climb_x_ = ledge.climb_x;
                ledge_platform_top_ = ledge.platform_top;
                ledge_hang_ticks_ = 0;
                tech_buffer_ticks_ = 0;
                hitstun_frames_ = 0;
                if (!ledge_invulnerability_used_) {
                    invuln_frames_ = std::max(
                        invuln_frames_, physics::kLedgeInvulnerabilityTicks);
                    ledge_invulnerability_used_ = true;
                }
                grounded_ = false;
            }
        }
    }

    if (grounded_) {
        up_special_used_ = false;
        air_dodge_used_ = false;
        ledge_invulnerability_used_ = false;
        jumps_remaining_ = 2;
    }

    // --- Animation driver tick --------------------------------------------
    const anim::Context actx{
        /*.grounded=*/ grounded_,
        /*.dead=*/ dead_,
        /*.attacking=*/ state_ == State::Attacking,
        /*.in_hitstun=*/ state_ == State::Hitstun,
        /*.horizontal_speed=*/ static_cast<float>(velocity_.x < 0 ? -velocity_.x : velocity_.x),
    };
    anim_driver_.tick(actx);
    if (sprite_) sprite_->set_current_frame(anim_driver_.global_frame());
}

Fighter::Snapshot Fighter::capture_state() const {
    return Snapshot{
        player_index_, body_, velocity_, di_reference_velocity_, grounded_, jumps_remaining_,
        jump_buffer_ticks_, coyote_ticks_, dead_, damage_percent_, facing_,
        state_, current_attack_id_, attack_frame_, attack_has_hit_,
        hitstun_frames_, stocks_, invuln_frames_, shield_, guard_frames_,
        dodge_frame_, dodge_cooldown_, dodge_dir_, surge_meter_, surge_frames_,
        parry_flash_, up_special_used_, air_dodge_used_,
        ledge_invulnerability_used_, tech_buffer_ticks_,
        tech_frames_, ledge_lockout_ticks_, ledge_hang_ticks_,
        ledge_climb_x_, ledge_platform_top_, anim_driver_.capture()
    };
}

bool Fighter::restore_state(const Snapshot& snapshot) {
    if (snapshot.player_index != player_index_
        || static_cast<int>(snapshot.state) < static_cast<int>(State::Idle)
        || static_cast<int>(snapshot.state) > static_cast<int>(State::LedgeHang)
        || static_cast<int>(snapshot.current_attack_id) < static_cast<int>(AttackId::None)
        || static_cast<int>(snapshot.current_attack_id) >= static_cast<int>(AttackId::Count)
        || !std::isfinite(snapshot.body.x) || !std::isfinite(snapshot.body.y)
        || !std::isfinite(snapshot.body.w) || !std::isfinite(snapshot.body.h)
        || !std::isfinite(snapshot.velocity.x) || !std::isfinite(snapshot.velocity.y)
        || !std::isfinite(snapshot.di_reference_velocity.x)
        || !std::isfinite(snapshot.di_reference_velocity.y)
        || !std::isfinite(snapshot.ledge_climb_x)
        || !std::isfinite(snapshot.ledge_platform_top)
        || snapshot.body.w <= 0.0f || snapshot.body.h <= 0.0f
        || (snapshot.facing != -1 && snapshot.facing != 1)
        || snapshot.jumps_remaining < 0 || snapshot.jumps_remaining > 2
        || snapshot.jump_buffer_ticks < 0 || snapshot.coyote_ticks < 0
        || snapshot.damage_percent < 0 || snapshot.damage_percent > combat::kMaxDamagePercent
        || snapshot.attack_frame < 0 || snapshot.hitstun_frames < 0
        || snapshot.stocks < 0 || snapshot.invuln_frames < 0
        || snapshot.shield < 0 || snapshot.guard_frames < 0
        || snapshot.dodge_frame < 0 || snapshot.dodge_cooldown < 0
        || snapshot.dodge_dir < -1 || snapshot.dodge_dir > 1
        || snapshot.shield > combat::kShieldMax
        || snapshot.surge_meter < 0 || snapshot.surge_meter > combat::kSurgeMeterMax
        || snapshot.surge_frames < 0 || snapshot.parry_flash < 0
        || snapshot.tech_buffer_ticks < 0
        || snapshot.tech_buffer_ticks > physics::kTechInputBufferTicks
        || snapshot.tech_frames < 0 || snapshot.tech_frames >= physics::kTechDurationTicks
        || snapshot.ledge_lockout_ticks < 0
        || snapshot.ledge_lockout_ticks > physics::kLedgeRegrabLockoutTicks
        || snapshot.ledge_hang_ticks < 0
        || snapshot.ledge_hang_ticks >= physics::kLedgeHangMaxTicks
        || (snapshot.state == State::Attacking
            && (snapshot.current_attack_id == AttackId::None
                || snapshot.attack_frame >= attack_total_frames(
                    get_attack(snapshot.current_attack_id))))
        || (snapshot.state != State::Attacking
            && snapshot.current_attack_id != AttackId::None)) {
        return false;
    }

    // Validate animation data before mutating the fighter.
    anim::Driver candidate;
    if (!candidate.restore(snapshot.animation)) return false;

    body_ = snapshot.body;
    velocity_ = snapshot.velocity;
    di_reference_velocity_ = snapshot.di_reference_velocity;
    grounded_ = snapshot.grounded;
    jumps_remaining_ = snapshot.jumps_remaining;
    jump_buffer_ticks_ = snapshot.jump_buffer_ticks;
    coyote_ticks_ = snapshot.coyote_ticks;
    dead_ = snapshot.dead;
    damage_percent_ = snapshot.damage_percent;
    facing_ = snapshot.facing;
    state_ = snapshot.state;
    current_attack_id_ = snapshot.current_attack_id;
    attack_frame_ = snapshot.attack_frame;
    attack_has_hit_ = snapshot.attack_has_hit;
    hitstun_frames_ = snapshot.hitstun_frames;
    stocks_ = snapshot.stocks;
    invuln_frames_ = snapshot.invuln_frames;
    shield_ = snapshot.shield;
    guard_frames_ = snapshot.guard_frames;
    dodge_frame_ = snapshot.dodge_frame;
    dodge_cooldown_ = snapshot.dodge_cooldown;
    dodge_dir_ = snapshot.dodge_dir;
    surge_meter_ = snapshot.surge_meter;
    surge_frames_ = snapshot.surge_frames;
    parry_flash_ = snapshot.parry_flash;
    up_special_used_ = snapshot.up_special_used;
    air_dodge_used_ = snapshot.air_dodge_used;
    ledge_invulnerability_used_ = snapshot.ledge_invulnerability_used;
    tech_buffer_ticks_ = snapshot.tech_buffer_ticks;
    tech_frames_ = snapshot.tech_frames;
    ledge_lockout_ticks_ = snapshot.ledge_lockout_ticks;
    ledge_hang_ticks_ = snapshot.ledge_hang_ticks;
    ledge_climb_x_ = snapshot.ledge_climb_x;
    ledge_platform_top_ = snapshot.ledge_platform_top;
    anim_driver_ = candidate;
    if (sprite_) sprite_->set_current_frame(anim_driver_.global_frame());
    return true;
}

void Fighter::apply_hit(int damage, Vec2 knockback_velocity, int hitstun) {
    if (dead_) return;
    // M5: invulnerable fighters ignore all incoming damage. Used to give the
    // respawning fighter a brief safe window to get back to the stage.
    if (invuln_frames_ > 0) return;
    damage_percent_ += damage;
    if (damage_percent_ > combat::kMaxDamagePercent) {
        damage_percent_ = combat::kMaxDamagePercent;
    }
    if (state_ == State::LedgeHang) {
        ledge_lockout_ticks_ = physics::kLedgeRegrabLockoutTicks;
        ledge_hang_ticks_ = 0;
        grounded_ = false;
    }
    tech_buffer_ticks_ = 0;
    tech_frames_ = 0;
    velocity_        = knockback_velocity;
    di_reference_velocity_ = knockback_velocity;
    hitstun_frames_  = hitstun < combat::kMinHitstunFrames
                       ? combat::kMinHitstunFrames
                       : hitstun;
    state_           = State::Hitstun;
    // Cancel any swing in progress; the hit lands regardless of the attack's
    // has_hit flag because the caller already verified the active hitbox.
    current_attack_id_ = AttackId::None;
    attack_frame_      = 0;
    attack_has_hit_    = false;
}

Aabb Fighter::active_hitbox() const {
    if (state_ != State::Attacking || attack_has_hit_) return {};
    const Attack& atk = get_attack(current_attack_id_);
    if (!attack_frame_is_active(atk, attack_frame_)) return {};
    // Mirror on x when facing left; the local hitbox is "facing-right".
    const float local_x = (facing_ > 0)
        ? atk.hitbox.x
        : (kWidth - atk.hitbox.x - atk.hitbox.w);
    return Aabb{
        body_.x + local_x,
        body_.y + atk.hitbox.y,
        atk.hitbox.w,
        atk.hitbox.h,
    };
}

void Fighter::render(Renderer& renderer) const {
    if (dead_) {
        return;
    }

    // M6-A: blit the procedural sprite. The sprite is drawn centered on
    // the body box (which is 48x64 source pixels), with the extra 8 px of
    // margin on each side giving room for the silhouette to extend (e.g.
    // attack frames in M6-B). The flip parameter mirrors the sprite so the
    // fighter faces the opponent without needing a second texture.
    if (sprite_) {
        const float sprite_w = static_cast<float>(
            Sprite::kSize * art::kRenderPixelSize);
        const float sprite_h = static_cast<float>(
            Sprite::kSize * art::kRenderPixelSize);
        // Anchor: place the sprite so its 48x64 body region lines up with
        // body_. The body_ is 48x64 in render-space; we offset the sprite
        // by half the (sprite - body) width to keep the body region
        // centered inside the sprite.
        const float dx = (sprite_w - body_.w) * 0.5f;
        const float dy = (sprite_h - body_.h) * 0.5f;
        renderer.draw_sprite(
            *sprite_,
            { body_.x - dx, body_.y - dy, sprite_w, sprite_h },
            art::kRenderPixelSize,
            /*flip_horizontal=*/facing_ < 0);
    }

    // M5: flashing outline during post-respawn invulnerability. The exact
    // pattern doesn't matter; this is a debug aid that ships in M5 to make
    // the invuln window visible without UI text. M6-A: now drawn around
    // the actual sprite (not the inner body box) so the outline doesn't
    // look like it's cutting off the silhouette.
    if (invuln_frames_ > 0) {
        const float sprite_w = static_cast<float>(
            Sprite::kSize * art::kRenderPixelSize);
        const float sprite_h = static_cast<float>(
            Sprite::kSize * art::kRenderPixelSize);
        const float dx = (sprite_w - body_.w) * 0.5f;
        const float dy = (sprite_h - body_.h) * 0.5f;
        renderer.draw_rect_outline(
            { body_.x - dx, body_.y - dy, sprite_w, sprite_h },
            config::kHudTextColor);
    }

    // M6-C: M3's active-hitbox debug outline is gone. The hit-spark
    // (rendered by Match from last_hit_ / hit_spark_age_) communicates
    // the same thing at the moment the player cares about it - a hit
    // connecting. The hitbox AABB itself is still used by Match's hit
    // resolver; this is a render-only cleanup.
}

} // namespace ember
