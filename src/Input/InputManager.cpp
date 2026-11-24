#include "Input/InputManager.h"

#include "Core/Logger.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

#include <algorithm>
#include <cstring>

namespace ember {


int InputManager::key_index(SDL_Keycode key) {
    constexpr int kSentinel = kMaxKeys - 1;
    const long long k = static_cast<long long>(key);
    if (k < 0) return kSentinel;
    if (k < kPlainKeyLimit) return static_cast<int>(k);
    if ((k & kScancodeBit) != 0 && (k & ~static_cast<long long>(kScancodeBit)) < kPlainKeyLimit) {
        return kPlainKeyLimit + static_cast<int>(k & ~static_cast<long long>(kScancodeBit));
    }
    return kSentinel;
}

SDL_Keycode InputManager::key_from_index(int index) {
    if (index < 0 || index >= kMaxKeys) return 0;
    if (index < kPlainKeyLimit) return static_cast<SDL_Keycode>(index);
    return static_cast<SDL_Keycode>(kScancodeBit | (index - kPlainKeyLimit));
}

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
                const int idx = key_index(event.key.key);
                if (event.key.repeat == 0) {
                    current_[idx] = true;
                }
                any_current_ = true;
                break;
            }

            case SDL_EVENT_KEY_UP: {
                const int idx = key_index(event.key.key);
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
    return current_[key_index(key)];
}

bool InputManager::pressed(SDL_Keycode key) const {
    const int idx = key_index(key);
    return current_[idx] && !previous_[idx];
}

bool InputManager::released(SDL_Keycode key) const {
    const int idx = key_index(key);
    return !current_[idx] && previous_[idx];
}

bool InputManager::any_held() const {
    return any_current_;
}

SDL_Keycode InputManager::any_pressed_key() const {
    // Find the lowest-index slot that transitioned from up to
    // down on the most recent update(). Returning the lowest
    // index is deterministic and matches the order in which
    // the keys appear in `current_`; the remap UI does not
    // care which one is reported first, only that the press
    // is consumed and the next call returns 0 until another
    // transition happens.
    for (int i = 0; i < kMaxKeys; ++i) {
        if (current_[i] && !previous_[i]) {
            return key_from_index(i);
        }
    }
    return 0;
}

// -----------------------------------------------------------------
// Menu nav predicates. We compute the OR over the keyboard edge
// transitions; the gamepad equivalent lives in Gamepad.h and is
// merged in App by ORing the two sources. The reason this is split
// (keyboard here, gamepad in App) is that the keyboard layer is
// the one that owns the SDL event queue + the test seam; the
// gamepad layer owns its own state. Both surfaces are merged
// in App::update_menu() into the menu's single "any input"
// predicate.
// -----------------------------------------------------------------
bool InputManager::menu_up    () const {
    return pressed(SDLK_W) || pressed(SDLK_UP);
}
bool InputManager::menu_down  () const {
    return pressed(SDLK_S) || pressed(SDLK_DOWN);
}
bool InputManager::menu_left  () const {
    return pressed(SDLK_A) || pressed(SDLK_LEFT);
}
bool InputManager::menu_right () const {
    return pressed(SDLK_D) || pressed(SDLK_RIGHT);
}
bool InputManager::menu_confirm() const {
    return pressed(SDLK_RETURN) || pressed(SDLK_SPACE);
}
bool InputManager::menu_cancel () const {
    return pressed(SDLK_ESCAPE);
}
bool InputManager::menu_pause  () const {
    return pressed(SDLK_P);
}
bool InputManager::menu_any_pressed() const {
    return menu_up() || menu_down() || menu_left() || menu_right()
        || menu_confirm() || menu_cancel() || menu_pause();
}

} // namespace ember