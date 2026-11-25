#include "Physics/Motion.h"

#include <algorithm>

namespace ember {

// Fixed tick rate the per-tick tuning constants assume. Per spec §9, all
// physics values are expressed in px/frame at 60 Hz. We don't currently
// integrate a variable-dt path in M2; the per-tick constants are used as-is
// and the caller is expected to invoke step_motion once per render frame.
// dt is accepted so the signature is stable for a future fixed-timestep loop.
MotionResult step_motion(const MotionState& current, const PlayerIntent& intent, double dt,
                        MotionTuning tuning) {
    (void)dt; // The caller advances this function exactly once per 60 Hz simulation tick.
    MotionResult next;
    next.velocity = current.velocity;
    next.grounded = current.grounded;
    next.jumps_remaining = current.jumps_remaining;
    next.jump_buffer_ticks = intent.jump_pressed
        ? physics::kJumpBufferTicks : std::max(0, current.jump_buffer_ticks - 1);
    next.coyote_ticks = current.grounded
        ? physics::kCoyoteTicks : std::max(0, current.coyote_ticks - 1);

    const double speed = physics::kWalkSpeed * tuning.walk
        * (intent.run_held ? physics::kRunSpeedMultiplier : 1.0);
    double target_vx = 0.0;
    if (intent.move_left) target_vx -= speed;
    if (intent.move_right) target_vx += speed;

    if (next.grounded) {
        next.velocity.x = target_vx;
        next.jumps_remaining = 2;
    } else {
        const double max_delta = physics::kAirControl * physics::kWalkSpeed * tuning.walk;
        if (next.velocity.x < target_vx)
            next.velocity.x = std::min(next.velocity.x + max_delta, target_vx);
        else if (next.velocity.x > target_vx)
            next.velocity.x = std::max(next.velocity.x - max_delta, target_vx);
    }

    // Buffered jump can fire on landing; coyote time permits a late first jump.
    if (next.jump_buffer_ticks > 0 && (next.grounded || next.coyote_ticks > 0)
        && next.jumps_remaining >= 1) {
        next.velocity.y = physics::kFirstJumpVelocity * tuning.jump;
        next.jumps_remaining = std::max(0, next.jumps_remaining - 1);
        next.grounded = false;
        next.coyote_ticks = 0;
        next.jump_buffer_ticks = 0;
    } else if (next.jump_buffer_ticks > 0 && !next.grounded && next.jumps_remaining > 0) {
        next.velocity.y = physics::kDoubleJumpVelocity * tuning.jump;
        --next.jumps_remaining;
        next.jump_buffer_ticks = 0;
    }

    double gravity = physics::kGravity;
    if (intent.fast_fall_held && next.velocity.y > 0.0)
        gravity *= physics::kFastFallMultiplier;
    next.velocity.y = std::min(next.velocity.y + gravity, physics::kTerminalVelocity);
    return next;
}

} // namespace ember
