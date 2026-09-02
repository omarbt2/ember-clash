#include "Input/InputManager.h"

#include "Core/Logger.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

#include <algorithm>
#include <cstring>

namespace ember {

namespace {

// Map an SDL_Keycode (which is platform-stable: SDLK_* / SDL_KeyCode values)
// into a small array index. SDL_Keycode is a 32-bit value, but most of the
// range is unused; we bucket anything below kMaxKeys directly and clamp
// larger values to a sentinel slot.
int bucket_index(SDL_Keycode key) {
    constexpr int kSentinel = InputManager::kMaxKeys - 1;
    if (key < 0) return kSentinel;
    const int idx = static_cast<int>(key);
    if (idx >= InputManager::kMaxKeys) return kSentinel;
    return idx;
}

} // namespace

InputManager::InputManager() {
    std::memset(current_,  0, sizeof(current_));
    std::memset(previous_, 0, sizeof(previous_));
}

void InputManager::update() {
    std::memcpy(previous_, current_, sizeof(current_));
    any_previous_ = any_current_;
    any_current_  = false;
    quit_requested_ = false;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                quit_requested_ = true;
                break;

            case SDL_EVENT_KEY_DOWN: {
                const int idx = bucket_index(event.key.key);
                if (event.key.repeat == 0) {
                    current_[idx] = true;
                }
                any_current_ = true;
                break;
            }

            case SDL_EVENT_KEY_UP: {
                const int idx = bucket_index(event.key.key);
                current_[idx] = false;
                // any_current_ may still be true from other held keys.
                break;
            }

            default:
                break;
        }
    }

    // If a key is held going into update() via SDL_PollEvent being starved,
    // we still want pressed() to fire exactly once.
    if (!any_previous_ && !any_current_) {
        // nothing was pressed; previous_ still reflects the prior frame.
    }
}

bool InputManager::held(SDL_Keycode key) const {
    return current_[bucket_index(key)];
}

bool InputManager::pressed(SDL_Keycode key) const {
    const int idx = bucket_index(key);
    return current_[idx] && !previous_[idx];
}

bool InputManager::released(SDL_Keycode key) const {
    const int idx = bucket_index(key);
    return !current_[idx] && previous_[idx];
}

bool InputManager::any_held() const {
    return any_current_;
}

} // namespace ember