#include "Input/InputManager.h"

#include "Core/Logger.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

#include <algorithm>
#include <cstring>

namespace ember {


int InputManager::key_index(SDL_Keycode key) {
    const long long k = static_cast<long long>(key);
    if (k < kPlainKeyFloor) return kInvalidKeyIndex; // Unknown/control codes are not bindable.
    if (k < kPlainKeyLimit) {
        // ASCII DEL and the C1 control block are not printable keys.
        if (k >= 127 && k < 160) return kInvalidKeyIndex;
        return static_cast<int>(k);
    }

    const long long mask = static_cast<long long>(kScancodeBit);
    if ((k & mask) != 0) {
        const long long scancode = k & ~mask;
        if (scancode > 0 && scancode < kScancodeCount) {
            return kPlainKeyLimit + static_cast<int>(scancode);
        }
    }
    return kInvalidKeyIndex;
}

SDL_Keycode InputManager::key_from_index(int index) {
    if (index < 0 || index >= kInvalidKeyIndex) return 0;
    if (index < kPlainKeyLimit) return static_cast<SDL_Keycode>(index);
    return static_cast<SDL_Keycode>(kScancodeBit | (index - kPlainKeyLimit));
}

bool InputManager::is_valid_keycode(SDL_Keycode key) {
    return key_index(key) != kInvalidKeyIndex;
}

InputManager::InputManager() {
    std::memset(current_,  0, sizeof(current_));
    std::memset(previous_, 0, sizeof(previous_));
}

void InputManager::update() {
    std::memcpy(previous_, current_, sizeof(current_));
    quit_requested_ = false;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                quit_requested_ = true;
                break;

            case SDL_EVENT_KEY_DOWN: {
                const int idx = key_index(event.key.key);
                if (idx != kInvalidKeyIndex && event.key.repeat == 0) {
                    current_[idx] = true;
                }
                break;
            }

            case SDL_EVENT_KEY_UP: {
                const int idx = key_index(event.key.key);
                if (idx != kInvalidKeyIndex) current_[idx] = false;
                break;
            }

            default:
                break;
        }
    }
}

bool InputManager::held(SDL_Keycode key) const {
    const int idx = key_index(key);
    return idx != kInvalidKeyIndex && current_[idx];
}

bool InputManager::pressed(SDL_Keycode key) const {
    const int idx = key_index(key);
    return idx != kInvalidKeyIndex && current_[idx] && !previous_[idx];
}

bool InputManager::released(SDL_Keycode key) const {
    const int idx = key_index(key);
    return idx != kInvalidKeyIndex && !current_[idx] && previous_[idx];
}

bool InputManager::any_held() const {
    for (int i = 0; i < kInvalidKeyIndex; ++i) {
        if (current_[i]) return true;
    }
    return false;
}

SDL_Keycode InputManager::any_pressed_key() const {
    // Find the lowest-index slot that transitioned from up to
    // down on the most recent update(). Returning the lowest
    // index is deterministic and matches the order in which
    // the keys appear in `current_`; the remap UI does not
    // care which one is reported first, only that the press
    // is consumed and the next call returns 0 until another
    // transition happens.
    for (int i = 0; i < kInvalidKeyIndex; ++i) {
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
    // Escape is reserved for pausing during a match and for backing
    // out of menu screens. P remains available for the P1 Dodge action.
    return pressed(SDLK_ESCAPE);
}
bool InputManager::menu_any_pressed() const {
    return menu_up() || menu_down() || menu_left() || menu_right()
        || menu_confirm() || menu_cancel() || menu_pause();
}

} // namespace ember