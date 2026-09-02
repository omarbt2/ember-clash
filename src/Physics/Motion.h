#pragma once

#include "Core/Math.h"
#include "Physics/PhysicsConfig.h"

// -----------------------------------------------------------------------------
// Pure motion step.
//
// All gameplay-relevant tuning (gravity, walk speed, jump velocity, air
// control, terminal velocity, fast-fall multiplier) is read from
// PhysicsConfig.h so that retuning a single constant changes the whole
// simulation consistently.
//
// The function takes the current state by value and returns the new state; it
// has no side effects. It does NOT do collision; the caller is expected to
// resolve collisions afterwards and may further mutate the returned velocity
// (e.g. zero it on landing).
// -----------------------------------------------------------------------------

namespace ember {

struct PlayerIntent {
    bool move_left   = false;
    bool move_right  = false;
    bool jump_pressed = false;  // rising edge this frame
    bool fast_fall_held = false;
};

struct MotionState {
    Vec2  velocity{0.0, 0.0};
    bool  grounded{false};
    int   jumps_remaining{2};
};

struct MotionResult {
    Vec2  velocity{0.0, 0.0};
    bool  grounded{false};
    int   jumps_remaining{2};
};

// One simulation tick. dt is in seconds; the function scales the per-tick
// physics values by (dt / kFixedStep) so behavior is the same whether the
// game runs at 60 Hz, 120 Hz, or variable.
MotionResult step_motion(const MotionState& current, const PlayerIntent& intent, double dt);

} // namespace ember
