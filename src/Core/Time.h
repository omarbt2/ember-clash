#pragma once

namespace ember {

// Tracks wall-clock time between frames and exposes a capped delta in seconds.
// Initialize once before the loop, then call update() at the top of each
// iteration to refresh delta_seconds().
class Clock {
public:
    Clock();

    // Mark the start of the next frame and update delta_seconds().
    void update();

    // Seconds elapsed since the previous update() call (clamped).
    double delta_seconds() const { return delta_; }

    // Seconds since the clock was constructed; useful for debug HUDs.
    double elapsed_seconds() const;

    // Target step duration derived from the configured FPS.
    double fixed_step_seconds() const;

private:
    double start_ticks_{0.0};
    double last_ticks_{0.0};
    double delta_{0.0};
};

} // namespace ember