#include "Physics/Motion.h"

#include <cassert>
#include <cmath>

using namespace ember;

int main() {
    // Ground jump consumes one jump and clears the input buffer.
    MotionState grounded{};
    grounded.grounded = true;
    PlayerIntent jump{};
    jump.jump_pressed = true;
    const MotionResult first = step_motion(grounded, jump);
    assert(first.velocity.y < 0.0);
    assert(first.jumps_remaining == 1);
    assert(first.jump_buffer_ticks == 0);

    // A late jump remains valid inside the six-tick coyote window.
    MotionState coyote{};
    coyote.grounded = false;
    coyote.jumps_remaining = 2;
    coyote.coyote_ticks = physics::kCoyoteTicks - 1;
    const MotionResult late = step_motion(coyote, jump);
    assert(late.velocity.y < 0.0);
    assert(late.jumps_remaining == 1);
    assert(late.coyote_ticks == 0);

    // The same input after the coyote window expires becomes a double jump.
    MotionState expired{};
    expired.grounded = false;
    expired.jumps_remaining = 1;
    expired.coyote_ticks = 0;
    const MotionResult double_jump = step_motion(expired, jump);
    assert(double_jump.velocity.y < 0.0);
    assert(double_jump.jumps_remaining == 0);

    // Jump input is buffered for six ticks and fires on the next grounded tick.
    MotionState airborne{};
    airborne.grounded = false;
    airborne.jumps_remaining = 0;
    airborne.jump_buffer_ticks = 3;
    const MotionResult buffered = step_motion(airborne, PlayerIntent{});
    assert(buffered.jump_buffer_ticks == 2);
    MotionState landing{};
    landing.grounded = true;
    landing.jumps_remaining = 2;
    landing.jump_buffer_ticks = buffered.jump_buffer_ticks;
    const MotionResult auto_jump = step_motion(landing, PlayerIntent{});
    assert(auto_jump.velocity.y < 0.0);
    assert(auto_jump.jump_buffer_ticks == 0);

    // Run modifier is deterministic and increases grounded horizontal speed.
    MotionState on_ground{};
    on_ground.grounded = true;
    PlayerIntent walk{};
    walk.move_right = true;
    PlayerIntent run = walk;
    run.run_held = true;
    const auto walk_result = step_motion(on_ground, walk);
    const auto run_result = step_motion(on_ground, run);
    assert(run_result.velocity.x > walk_result.velocity.x);

    // Contradictory horizontal input cancels out instead of choosing a side.
    PlayerIntent neutral = walk;
    neutral.move_left = true;
    const auto neutral_result = step_motion(on_ground, neutral);
    assert(std::abs(neutral_result.velocity.x) < 1e-9);
    return 0;
}
