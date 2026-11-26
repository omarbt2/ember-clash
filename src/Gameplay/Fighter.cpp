#include "Gameplay/Fighter.h"

#include "Gameplay/CombatConfig.h"
#include "Graphics/Renderer.h"
#include "Physics/PhysicsConfig.h"
#include "World/Arena.h"

namespace ember {

namespace {
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
    if (dead_) {
        return;
    }

    // --- Invulnerability decay (M5) ----------------------------------------
    if (invuln_frames_ > 0) {
        --invuln_frames_;
    }

    // --- Timers (Rebirth) ---------------------------------------------------
    if (dodge_cooldown_ > 0) --dodge_cooldown_;
    if (parry_flash_ > 0)    --parry_flash_;
    if (surge_frames_ > 0)   --surge_frames_;

    // --- State advance -----------------------------------------------------
    if (state_ == State::Hitstun) {
        if (--hitstun_frames_ <= 0) {
            hitstun_frames_ = 0;
            state_          = State::Idle;
        }
    }
    if (state_ == State::Attacking) {
        advance_attack();
    }
    if (state_ == State::Dodge) {
        if (++dodge_frame_ >= combat::kDodgeFrames) {
            dodge_frame_ = 0;
            state_ = State::Idle;
        }
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

    // --- Ember Surge activation ---------------------------------------------
    if (combat_intent.surge_pressed && surge_frames_ == 0
        && surge_meter_ >= combat::kSurgeMeterMax
        && state_ != State::Hitstun) {
        surge_meter_  = 0;
        surge_frames_ = combat::kSurgeFrames;
    }

    // --- Dodge (from Idle or Guard) ----------------------------------------
    if ((state_ == State::Idle || state_ == State::Guard)
        && combat_intent.dodge_pressed && dodge_cooldown_ == 0
        && (grounded_ || !air_dodge_used_)) {
        state_        = State::Dodge;
        dodge_frame_  = 0;
        guard_frames_ = 0;
        dodge_cooldown_ = combat::kDodgeCooldownFrames;
        dodge_dir_ = motion_intent.move_right && !motion_intent.move_left ?  1
                   : motion_intent.move_left  && !motion_intent.move_right ? -1 : 0;
        if (!grounded_) air_dodge_used_ = true;
    }

    // --- Try to start a new attack / guard from Idle -----------------------
    if (state_ == State::Idle) {
        if (!try_start_attack(motion_intent, combat_intent)
            && combat_intent.shield_held && grounded_ && shield_ > 0) {
            state_        = State::Guard;
            guard_frames_ = 1;
        }
    }

    // --- Facing update (only when idle; attacks and hitstun lock facing) ---
    if (state_ == State::Idle) {
        if (motion_intent.move_left  && !motion_intent.move_right) facing_ = -1;
        if (motion_intent.move_right && !motion_intent.move_left)  facing_ =  1;
    }

    // --- Motion ------------------------------------------------------------
    if (state_ == State::Idle) {
        const bool drop_through = motion_intent.fast_fall_held
            && motion_intent.jump_pressed && grounded_
            && arena.supported_by_one_way(body_);
        PlayerIntent effective_intent = motion_intent;
        if (drop_through) effective_intent.jump_pressed = false;
        const MotionState in{velocity_, grounded_, jumps_remaining_, jump_buffer_ticks_, coyote_ticks_};
        const MotionResult out = step_motion(in, effective_intent);
        velocity_        = out.velocity;
        jumps_remaining_ = out.jumps_remaining;
        jump_buffer_ticks_ = out.jump_buffer_ticks;
        coyote_ticks_ = out.coyote_ticks;
    } else if (state_ == State::Guard) {
        velocity_.x *= 0.5;
        velocity_ = apply_gravity_only(velocity_, false);
    } else if (state_ == State::Dodge) {
        velocity_.x = combat::kDodgeSpeed * dodge_dir_;
        if (grounded_) {
            velocity_ = apply_gravity_only(velocity_, false);
        } else {
            velocity_.y = 0.0;   // air dodge hovers for its duration
        }
    } else {
        // Attacking or Hitstun: gravity only; horizontal momentum is kept
        // (the M7 AI is tuned around this slide; revisit in the AI pass).
        velocity_ = apply_gravity_only(velocity_, motion_intent.fast_fall_held);
    }

    // --- Integrate position + resolve collision ----------------------------
    // High-speed knockback is advanced in bounded substeps so a fighter
    // cannot tunnel through a platform in one large per-tick displacement.
    const bool drop_through = state_ == State::Idle
        && motion_intent.fast_fall_held && motion_intent.jump_pressed
        && arena.supported_by_one_way(body_);
    grounded_ = arena.move_and_resolve(body_, velocity_, drop_through);
    if (grounded_) { up_special_used_ = false; air_dodge_used_ = false; jumps_remaining_ = 2; }
    // M5 note: the death-zone check used to live here, but it needed to
    // happen BEFORE Fighter::update so Match could still decrement
    // stocks and start the respawn timer in the same frame. Match now owns
    // the alive->dead transition; Fighter::update just leaves dead_ alone.

    // --- M6-B: animation driver tick --------------------------------------
    // Tick the driver with the freshly-resolved grounded flag and the
    // post-attack / post-hitstun state, then pin the sprite to the
    // resulting global frame. We use the absolute value of horizontal
    // velocity for the walk threshold (movement direction doesn't
    // matter to the animation - "is the fighter running?" is enough).
    const anim::Context actx{
        /*.grounded=*/        grounded_,
        /*.dead=*/            dead_,
        /*.attacking=*/       state_ == State::Attacking,
        /*.in_hitstun=*/      state_ == State::Hitstun,
        /*.horizontal_speed=*/ static_cast<float>(velocity_.x < 0 ? -velocity_.x : velocity_.x),
    };
    anim_driver_.tick(actx);
    if (sprite_) sprite_->set_current_frame(anim_driver_.global_frame());
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
    velocity_        = knockback_velocity;
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
