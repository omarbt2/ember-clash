#pragma once

namespace ember {

// Tracks a polled button while preserving transitions until the simulation
// consumes them. Render/event polling can happen more often than simulation
// ticks (for example 120/144 Hz display vs a 60 Hz game tick), so a one-poll
// pressed/released edge must not disappear before gameplay sees it.
class InputEdgeState {
public:
    void poll(bool down) {
        previous_ = current_;
        if (down && !current_) pressed_latched_ = true;
        if (!down && current_) released_latched_ = true;
        current_ = down;
    }

    bool held() const { return current_; }
    bool pressed() const { return pressed_latched_ || (current_ && !previous_); }
    bool released() const { return released_latched_ || (!current_ && previous_); }

    // Called once after the complete simulation tick has processed all input
    // consumers (menu, match and training controls).
    void consume_edges() {
        pressed_latched_ = false;
        released_latched_ = false;
        previous_ = current_;
    }

    // Test seams retain the original synthetic-state semantics.
    void set_for_test(bool down) { current_ = down; }
    void begin_frame_for_test() {
        previous_ = current_;
        pressed_latched_ = false;
        released_latched_ = false;
    }
    void reset() {
        current_ = false;
        previous_ = false;
        pressed_latched_ = false;
        released_latched_ = false;
    }

private:
    bool current_{false};
    bool previous_{false};
    bool pressed_latched_{false};
    bool released_latched_{false};
};

} // namespace ember
