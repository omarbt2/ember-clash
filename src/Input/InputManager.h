#pragma once

#include <SDL3/SDL_keycode.h>

#include <cstring>

namespace ember {

// Tracks keyboard state for the current and previous frame.
//
// `held`    - key is currently down.
// `pressed` - transitioned from up to down this frame.
// `released`- transitioned from down to up this frame.
//
// The implementation polls SDL events each frame in InputManager::update(),
// so caller code only needs to query the predicates. Gamepad and touch
// input layers will plug in here later without disturbing gameplay code.
class InputManager {
public:
    // Maximum number of distinct scancode/keycode slots we track. Larger
    // keycodes are bucketed into the final slot.
    static constexpr int kMaxKeys = 512;

    InputManager();

    // Drains the SDL event queue, updates key state, refreshes quit flag.
    void update();

    bool quit_requested() const { return quit_requested_; }

    bool held   (SDL_Keycode key) const;
    bool pressed(SDL_Keycode key) const;
    bool released(SDL_Keycode key) const;

    // Returns true while any tracked key is down. Useful for things like
    // "any input to dismiss a menu" screens.
    bool any_held() const;

    // -----------------------------------------------------------------
    // M6-C test seam: drive the InputMap end-to-end from a headless
    // harness without bringing up an SDL window. Follows the same
    // pattern as Fighter::set_*_for_test, AiController::reset_for_test,
    // and the other test-only seams already in the codebase.
    //
    // Production callers should never touch this. Gameplay code uses
    // update() to drain SDL events. The harness uses these to inject
    // a synthetic keyboard state and call the real InputMap::sample
    // path, which is the only path the production code ever exercises.
    // -----------------------------------------------------------------
    void set_held_for_test(SDL_Keycode key, bool is_down) {
        const int idx = (key < 0) ? (kMaxKeys - 1) : (key >= kMaxKeys ? (kMaxKeys - 1) : key);
        current_[idx] = is_down;
        any_current_ = any_current_ || is_down;
    }
    // Mark the current state as the previous frame's state, so the next
    // call to pressed(key) returns true only for keys that are held now
    // but weren't held at this snapshot. Mirrors the SDL_EVENT_KEY_DOWN
    // edge semantics without requiring an actual event.
    void begin_frame_for_test() {
        std::memcpy(previous_, current_, sizeof(previous_));
        any_previous_ = any_current_;
        quit_requested_ = false;
    }
    // Convenience: clear all held keys (used between scenarios).
    void clear_for_test() {
        std::memset(current_,  0, sizeof(current_));
        std::memset(previous_, 0, sizeof(previous_));
        any_current_  = false;
        any_previous_ = false;
    }

private:
    bool current_[kMaxKeys]{};
    bool previous_[kMaxKeys]{};
    bool any_current_{false};
    bool any_previous_{false};
    bool quit_requested_{false};
};

} // namespace ember