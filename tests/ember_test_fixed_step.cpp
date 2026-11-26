#include "Core/FixedStep.h"
#include "Core/InputEdgeState.h"

#include <cassert>
#include <cmath>
#include <limits>

int main() {
    using ember::FixedStepAccumulator;

    // 120 Hz rendering must produce exactly 60 simulation ticks per second.
    {
        FixedStepAccumulator clock;
        int ticks = 0;
        for (int frame = 0; frame < 120; ++frame) {
            ticks += clock.advance(1.0 / 120.0).ticks;
        }
        assert(ticks == 60);
        assert(std::abs(clock.remainder_seconds()) < 1e-9);
    }

    // 30 Hz rendering must catch up with two simulation ticks per frame.
    {
        FixedStepAccumulator clock;
        int ticks = 0;
        for (int frame = 0; frame < 30; ++frame) {
            ticks += clock.advance(1.0 / 30.0).ticks;
        }
        assert(ticks == 60);
        assert(std::abs(clock.remainder_seconds()) < 1e-9);
    }

    // 144 Hz rendering must also average exactly 60 simulation ticks/second.
    {
        FixedStepAccumulator clock;
        int ticks = 0;
        for (int frame = 0; frame < 144; ++frame) {
            ticks += clock.advance(1.0 / 144.0).ticks;
        }
        assert(ticks == 60);
        assert(std::abs(clock.remainder_seconds()) < 1e-9);
    }

    // A full maximum frame delta is capped at three ticks, with the rest
    // explicitly discarded to match the desktop stall policy.
    {
        FixedStepAccumulator clock;
        const auto result = clock.advance(1.0 / 15.0);
        assert(result.ticks == FixedStepAccumulator::kMaxTicksPerFrame);
        assert(result.dropped_seconds > 0.0);
        assert(std::abs(result.remainder_seconds) < 1e-9);
    }

    // When catch-up is capped, only whole excess ticks are dropped. A
    // fractional remainder is kept so the simulation phase remains stable.
    {
        FixedStepAccumulator clock;
        assert(clock.advance(1.0 / 120.0).ticks == 0);
        const auto result = clock.advance(1.0 / 15.0);
        assert(result.ticks == FixedStepAccumulator::kMaxTicksPerFrame);
        assert(result.dropped_seconds >= FixedStepAccumulator::kStepSeconds - 1e-9);
        assert(std::abs(result.remainder_seconds
            - FixedStepAccumulator::kStepSeconds * 0.5) < 1e-9);
    }

    // Invalid and negative deltas cannot poison the accumulator.
    {
        FixedStepAccumulator clock;
        assert(clock.advance(-1.0).ticks == 0);
        assert(clock.advance(std::numeric_limits<double>::quiet_NaN()).ticks == 0);
        assert(clock.advance(std::numeric_limits<double>::infinity()).ticks == 0);
        assert(std::abs(clock.remainder_seconds()) < 1e-9);
    }

    // A press survives several render polls until a simulation tick consumes it.
    {
        ember::InputEdgeState button;
        button.poll(true);
        button.poll(true);
        button.poll(true);
        assert(button.held());
        assert(button.pressed());
        button.consume_edges();
        assert(button.held());
        assert(!button.pressed());
    }

    // A short press and release between simulation ticks preserves both edges.
    {
        ember::InputEdgeState button;
        button.poll(true);
        button.poll(false);
        assert(!button.held());
        assert(button.pressed());
        assert(button.released());
        button.consume_edges();
        assert(!button.pressed());
        assert(!button.released());
    }

    return 0;
}
