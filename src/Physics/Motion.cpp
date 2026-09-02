#include "Physics/Motion.h"

#include <algorithm>

namespace ember {

// Fixed tick rate the per-tick tuning constants assume. Per spec §9, all
// physics values are expressed in px/frame at 60 Hz. We don't currently
// integrate a variable-dt path in M2; the per-tick constants are used as-is
// and the caller is expected to invoke step_motion once per render frame.
// dt is accepted so the signature is stable for a future fixed-timestep loop.
MotionResult step_motion(const MotionState& current, const PlayerIntent& intent, [[maybe_unused]] double dt) {
    MotionResult next;
    next.velocity        = current.velocity;
    next.grounded        = current.grounded;
    next.jumps_remaining = current.grounded ? 2 : current.jumps_remaining;

    // --- Horizontal velocity -------------------------------------------------
    double target_vx = 0.0;
    if (intent.move_left)  target_vx -= physics::kWalkSpeed;
    if (intent.move_right) target_vx += physics::kWalkSpeed;

    if (next.grounded) {
        next.velocity.x = target_vx;
    } else {
        // Air control: cap how much vx may change per tick.
        const double max_delta = physics::kAirControl * physics::kWalkSpeed;
        if (next.velocity.x < target_vx) {
            next.velocity.x = std::min(next.velocity.x + max_delta, target_vx);
        } else if (next.velocity.x > target_vx) {
            next.velocity.x = std::max(next.velocity.x - max_delta, target_vx);
        }
    }

    // --- Jump (edge-triggered) ----------------------------------------------
    if (intent.jump_pressed && next.jumps_remaining > 0 && !next.grounded) {
        next.velocity.y = (next.jumps_remaining == 2)
            ? physics::kFirstJumpVelocity
            : physics::kDoubleJumpVelocity;
        --next.jumps_remaining;
    } else if (intent.jump_pressed && next.grounded) {
        next.velocity.y = physics::kFirstJumpVelocity;
        next.jumps_remaining = 1;
        next.grounded = false;
    }

    // --- Gravity / fast-fall -------------------------------------------------
    double g = physics::kGravity;
    if (intent.fast_fall_held && next.velocity.y > 0.0) {
        g *= physics::kFastFallMultiplier;
    }
    next.velocity.y += g;
    if (next.velocity.y > physics::kTerminalVelocity) {
        next.velocity.y = physics::kTerminalVelocity;
    }

    return next;
}

} // namespace ember
