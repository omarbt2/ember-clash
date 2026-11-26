#pragma once

#include "Core/Math.h"

namespace ember {

// Identifies an attack. M3 ships a tiny subset (Jab, Forward Tilt, Neutral Air);
// the other moves from the design brief (Up Smash, Down Smash, Up Air, Special,
// Recovery Special) will be added in a follow-up. The "None" sentinel is what
// Fighter::current_attack_id_ holds when no attack is in progress.
enum class AttackId : int {
    None = 0,
    Jab,
    ForwardTilt,
    NeutralAir,
    // --- Rebirth Phase 2: full move set -----------------------------------
    UpTilt,
    DownTilt,
    ForwardSmash,
    UpSmash,
    DownSmash,
    ForwardAir,
    BackAir,
    UpAir,
    DownAir,
    NeutralSpecial,   // Ember Burst   - wide blast around the body
    SideSpecial,      // Flare Dash    - dashing strike
    UpSpecial,        // Rising Phoenix- recovery, once per airtime
    DownSpecial,      // Ground Slam   - dive / shockwave
    Count,
};

// Platform-agnostic attack intent. Built by InputMap each frame from
// whatever input source is active (keyboard today; gamepad / touch later).
// Edge-triggered: a button must transition from up to down within the
// last frame for *_pressed to be true. Held state is NOT exposed because
// every M3 attack is a single tap.
struct CombatIntent {
    bool jab_pressed   = false;
    bool ftilt_pressed = false;
    bool nair_pressed  = false;
    // Rebirth Phase 2 additions (all default false so older call sites that
    // brace-initialise only the first three fields keep compiling).
    bool special_pressed = false;  // special button (direction picks which)
    bool smash_held      = false;  // modifier: strong button becomes a smash
    bool shield_held     = false;  // guard (parry window on the first frames)
    bool shield_pressed  = false;  // edge: also used for the buffered landing tech
    bool dodge_pressed   = false;  // spot / directional dodge
    bool surge_pressed   = false;  // activate Ember Surge when the meter is full
};

// Pure data describing one attack. All values are starting points per the
// design brief; they're inlined into a static table in Attack.cpp so they
// can be tuned in one place.
//
// Coordinate system: hitbox is in fighter-LOCAL space, with the fighter's
// top-left at (0, 0) and the body extending to (Fighter::kWidth, kHeight).
// At runtime the world-space hitbox is `body.pos + facing * local_offset`
// where `facing` is +1 (right) or -1 (left); the hitbox is mirrored on x
// when facing left.
//
// Timing is in render frames (60 Hz). On startup + active + recovery the
// swing ends and the fighter returns to Idle.
struct Attack {
    const char* name;          // human-readable, for debug / future UI
    int         startup_frames;
    int         active_frames;
    int         recovery_frames;
    bool        requires_ground;   // true = must be grounded to start
    bool        requires_air;      // true = must be airborne to start
    int         damage;            // % added to the target on hit
    int         base_knockback;    // see CombatConfig::knockback formula
    int         hitstun_frames;    // how long the target is locked
    float       knockback_angle_x; // unit vector, x in attacker's local
    float       knockback_angle_y; // space; facing is applied at hit time
    Aabb        hitbox;            // local-space hitbox AABB
    // Rebirth Phase 2: optional self-propulsion applied when the attack
    // starts (dash / recovery specials). 0 means "leave velocity alone".
    float       self_vx = 0.0f;    // multiplied by facing
    float       self_vy = 0.0f;    // absolute (negative = up)
    bool        once_per_airtime = false;  // recovery specials
};

inline int attack_total_frames(const Attack& a) {
    return a.startup_frames + a.active_frames + a.recovery_frames;
}

// Structural validation for data-driven move tables. Keep this independent
// of SDL and rendering so every move can be checked in a headless test.
inline bool attack_is_well_formed(const Attack& a) {
    if (a.startup_frames < 0 || a.active_frames < 0 || a.recovery_frames < 0)
        return false;
    if (a.requires_ground && a.requires_air) return false;
    if (a.damage < 0 || a.base_knockback < 0 || a.hitstun_frames < 0)
        return false;
    if (a.hitbox.w < 0.0f || a.hitbox.h < 0.0f) return false;
    if (a.active_frames > 0 && (a.hitbox.w <= 0.0f || a.hitbox.h <= 0.0f))
        return false;
    if (a.once_per_airtime && a.requires_ground) return false;
    return true;
}

// True iff frame_index (0-based within the swing) falls inside the active
// window. Used by Match to decide whether to test for hitbox intersections.
inline bool attack_frame_is_active(const Attack& a, int frame_index) {
    return frame_index >= a.startup_frames
        && frame_index <  a.startup_frames + a.active_frames;
}

// Returns the attack with the given id, or the None-sentinel attack when id
// is out of range. The returned reference is into a static table.
const Attack& get_attack(AttackId id);

} // namespace ember
