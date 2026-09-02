#pragma once

#include <SDL3/SDL_keycode.h>

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

private:
    bool current_[kMaxKeys]{};
    bool previous_[kMaxKeys]{};
    bool any_current_{false};
    bool any_previous_{false};
    bool quit_requested_{false};
};

} // namespace ember