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
    bool up_held = false;       // Rebirth: aim for up tilts/smashes/airs/special
    bool run_held = false;      // Rebirth: run modifier while moving
};

// Rebirth Phase 3: per-character tuning applied on top of PhysicsConfig.
struct MotionTuning {
    double walk = 1.0;
    double jump = 1.0;
};

struct MotionState {
    Vec2  velocity{0.0, 0.0};
    bool  grounded{false};
    int   jumps_remaining{2};
    int   jump_buffer_ticks{0};
    int   coyote_ticks{0};
};

struct MotionResult {
    Vec2  velocity{0.0, 0.0};
    bool  grounded{false};
    int   jumps_remaining{2};
    int   jump_buffer_ticks{0};
    int   coyote_ticks{0};
};

// One simulation tick. dt is in seconds; the function scales the per-tick
// physics values by (dt / kFixedStep) so behavior is the same whether the
// game runs at 60 Hz, 120 Hz, or variable.
MotionResult step_motion(const MotionState& current, const PlayerIntent& intent, double dt,
                         MotionTuning tuning = {});

} // namespace ember
