#include "Core/Time.h"

#include "Core/Config.h"

#include <SDL3/SDL_timer.h>

#include <algorithm>

namespace ember {

namespace {

double ticks_to_seconds(std::uint64_t ticks) {
    // SDL_GetPerformanceCounter returns platform-dependent counter units.
    return static_cast<double>(ticks) / static_cast<double>(SDL_GetPerformanceFrequency());
}

} // namespace

Clock::Clock()
    : start_ticks_(ticks_to_seconds(SDL_GetPerformanceCounter()))
    , last_ticks_(start_ticks_)
{}

void Clock::update() {
    const double now     = ticks_to_seconds(SDL_GetPerformanceCounter());
    const double raw_d   = now - last_ticks_;
    const double max_d   = config::kMaxDeltaSeconds;
    delta_               = std::clamp(raw_d, 0.0, max_d);
    last_ticks_          = now;
}

double Clock::elapsed_seconds() const {
    return ticks_to_seconds(SDL_GetPerformanceCounter()) - start_ticks_;
}

double Clock::fixed_step_seconds() const {
    return 1.0 / static_cast<double>(config::kTargetFps);
}

} // namespace ember