#pragma once

#include "Core/Config.h"

#include <algorithm>
#include <cmath>

namespace ember {

// Converts an arbitrary render-frame delta into a bounded number of fixed
// simulation ticks. Gameplay constants are authored per 60 Hz tick; both the
// desktop and browser loops must use this exact accumulator.
class FixedStepAccumulator {
public:
    static constexpr int kMaxTicksPerFrame = 3;
    static constexpr double kStepSeconds =
        1.0 / static_cast<double>(config::kTargetFps);

    struct Result {
        int ticks{0};
        double dropped_seconds{0.0};
        double remainder_seconds{0.0};
    };

    Result advance(double frame_seconds) {
        if (!std::isfinite(frame_seconds) || frame_seconds < 0.0) {
            frame_seconds = 0.0;
        }
        accumulator_ += std::min(frame_seconds, config::kMaxDeltaSeconds);

        Result result{};
        constexpr double kEpsilon = 1e-12;
        while (result.ticks < kMaxTicksPerFrame
               && accumulator_ + kEpsilon >= kStepSeconds) {
            accumulator_ -= kStepSeconds;
            if (accumulator_ < 0.0 && accumulator_ > -kEpsilon) {
                accumulator_ = 0.0;
            }
            ++result.ticks;
        }

        // After the catch-up budget is exhausted, discard any additional
        // whole-tick backlog but retain a sub-tick remainder for interpolation.
        if (result.ticks == kMaxTicksPerFrame
            && accumulator_ + kEpsilon >= kStepSeconds) {
            // Drop only complete excess ticks; keep the fractional phase so
            // render cadence does not introduce an avoidable timing drift.
            const double retained = std::fmod(accumulator_, kStepSeconds);
            result.dropped_seconds = accumulator_ - retained;
            accumulator_ = retained < kEpsilon ? 0.0 : retained;
        }
        result.remainder_seconds = accumulator_;
        return result;
    }

    void reset() { accumulator_ = 0.0; }
    double remainder_seconds() const { return accumulator_; }

private:
    double accumulator_{0.0};
};

} // namespace ember
