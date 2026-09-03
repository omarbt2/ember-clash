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
      color_(player_index == 0 ? config::kFighterP1 : config::kFighterP2) {
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

    // First call (from the constructor) initializes stocks to the starting
    // value; subsequent calls (from Match on respawn) preserve the count so
    // we don't refill lives mid-match.
    if (stocks_ == 0) {
        stocks_ = combat::kStartingStocks;
    }
}

// Called by Match immediately after a respawn to grant the post-respawn
// invulnerability window. In M5 the duration is in seconds; we convert to
// frames at 60Hz here so the rest of Fighter can stay in fixed-step units.
void Fighter::set_respawn_invuln_seconds(double seconds) {
    if (seconds <= 0.0) {
        invuln_frames_ = 0;
        return;
    }
    // 60Hz fixed step; rounded up so even a 0.1s window is at least 1 frame.
    const int frames = static_cast<int>(seconds * 60.0 + 0.5);
    invuln_frames_ = frames;
}

// Used by Match::reset() to wipe a rematch's per-fighter state back to a
// fresh match. Resets stocks, damage, and any in-progress attack/hitstun.
void Fighter::reset_for_rematch() {
    stocks_        = combat::kStartingStocks;
    damage_percent_ = 0;
    dead_           = false;
    invuln_frames_  = 0;
    hitstun_frames_ = 0;
    state_          = State::Idle;
    current_attack_id_ = AttackId::None;
    attack_frame_      = 0;
    attack_has_hit_    = false;
}

bool Fighter::try_start_attack(const CombatIntent& ci) {
    // Determine the requested attack by priority. Higher-index attacks win
    // because they're the more deliberate inputs; nair is checked last so
    // a stray direction press can't cancel a stronger ground option.
    AttackId requested = AttackId::None;
    if (ci.nair_pressed)        requested = AttackId::NeutralAir;
    if (ci.ftilt_pressed)       requested = AttackId::ForwardTilt;
    if (ci.jab_pressed)         requested = AttackId::Jab;
    if (requested == AttackId::None) return false;

    const Attack& atk = get_attack(requested);
    if (atk.requires_ground && !grounded_) return false;
    if (atk.requires_air    &&  grounded_) return false;

    current_attack_id_ = requested;
    attack_frame_      = 0;
    attack_has_hit_    = false;
    state_             = State::Attacking;
    return true;
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
                     const Arena& arena,
                     double /*dt*/) {
    if (dead_) {
        return;
    }

    // --- Invulnerability decay (M5) ----------------------------------------
    if (invuln_frames_ > 0) {
        --invuln_frames_;
    }

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

    // --- Try to start a new attack from Idle -------------------------------
    if (state_ == State::Idle) {
        try_start_attack(combat_intent);
    }

    // --- Facing update (only when idle; attacks and hitstun lock facing) ---
    if (state_ == State::Idle) {
        if (motion_intent.move_left  && !motion_intent.move_right) facing_ = -1;
        if (motion_intent.move_right && !motion_intent.move_left)  facing_ =  1;
    }

    // --- Motion ------------------------------------------------------------
    if (state_ == State::Idle) {
        // Full motion step. step_motion() respects air control, fast-fall,
        // and jump; grounded_ is refreshed by collision below.
        const MotionState in{velocity_, grounded_, jumps_remaining_};
        const MotionResult out = step_motion(in, motion_intent, /*dt*/ 1.0 / 60.0);
        velocity_        = out.velocity;
        jumps_remaining_ = out.jumps_remaining;
    } else {
        // Attacking or Hitstun: gravity only, preserve horizontal velocity.
        // Fast-fall is allowed during attack startup/active/recovery so the
        // player can accelerate a falling aerial; spec's M3 spec doesn't
        // require it but it costs nothing and feels better in the hand.
        velocity_ = apply_gravity_only(velocity_, motion_intent.fast_fall_held);
    }

    // --- Integrate position ------------------------------------------------
    body_.x += static_cast<float>(velocity_.x);
    body_.y += static_cast<float>(velocity_.y);

    // --- Resolve arena collision -------------------------------------------
    grounded_ = arena.resolve_collision(body_, velocity_);
    // M5 note: the death-zone check used to live here, but it needed to
    // happen BEFORE Fighter's own update so Match could still decrement
    // stocks and start the respawn timer in the same frame. Match now owns
    // the alive->dead transition; Fighter::update just leaves dead_ alone.
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
    if (state_ != State::Attacking) return {};
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
    renderer.draw_filled_rect(
        {body_.x, body_.y, body_.w, body_.h},
        color_);
    // M5: flashing outline during post-respawn invulnerability. The exact
    // pattern doesn't matter; this is a debug aid that ships in M5 to make
    // the invuln window visible without UI text.
    const config::Color outline = (invuln_frames_ > 0)
        ? config::kHudTextColor
        : config::kAccentColor;
    renderer.draw_rect_outline(
        {body_.x, body_.y, body_.w, body_.h},
        outline);

    // Active hitbox overlay (M3 debug aid; M6 will replace with real art).
    if (state_ == State::Attacking) {
        const Aabb hb = active_hitbox();
        if (hb.w > 0.0f && hb.h > 0.0f) {
            renderer.draw_rect_outline(
                {hb.x, hb.y, hb.w, hb.h},
                config::kCombatHitboxColor);
        }
    }
}

} // namespace ember
