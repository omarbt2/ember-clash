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
    // Slot table layout:
    //   [0, 512)       plain keycodes below 512 (ASCII keys)
    //   [512, 1024)    SDL scancode-masked keycodes (arrows, keypad,
    //                  modifiers, function keys), slot = 512 + scancode
    //   [1024]         invalid-key sentinel; never used as a real key.
    // Before this layout every keycode >= 512 collapsed into ONE slot, so
    // e.g. pressing Left also read as Right/Up/Down and Shift aliased with
    // the whole keypad - which made arrow-key local 2P unplayable.
    static constexpr int kPlainKeyFloor = 32;
    static constexpr int kPlainKeyLimit = 512;
    static constexpr int kScancodeCount = 512;
    static constexpr int kInvalidKeyIndex = kPlainKeyLimit + kScancodeCount;
    static constexpr int kMaxKeys = kInvalidKeyIndex + 1;
    static constexpr int kScancodeBit   = 1 << 30;

    // Maps a keycode to its slot, and back. key_from_index() is the exact
    // inverse for every slot that key_index() can produce for a real key.
    static int        key_index(SDL_Keycode key);
    static SDL_Keycode key_from_index(int index);
    static bool       is_valid_keycode(SDL_Keycode key);

    InputManager();

    // Drains the SDL event queue, updates key state, refreshes quit flag.
    void update();

    bool quit_requested() const { return quit_requested_; }

    bool held   (SDL_Keycode key) const;
    bool pressed(SDL_Keycode key) const;
    bool released(SDL_Keycode key) const;

    // Returns true while any tracked key is down, including keys held across
    // frames where SDL emits no new key-down event.
    bool any_held() const;

    // Returns the first keycode whose state transitioned from
    // up to down on the most recent update(), or 0 if no key
    // is newly down. Used by the M13 remap UI to capture a
    // single keypress. Edge-triggered: the function reports
    // the key only on the frame the transition happens.
    SDL_Keycode any_pressed_key() const;

    // -----------------------------------------------------------------
    // M12 menu-navigation predicates. These merge keyboard + gamepad
    // into a single "menu input" surface so the menu code does not
    // need to know which device the player is using. The keys are
    // WASD + arrow keys + Enter + Esc; the gamepad equivalents are
    // d-pad + left stick + A/B/Start/Back. Edge-triggered (pressed
    // = true only on the transition from up to down).
    // -----------------------------------------------------------------
    bool menu_up    () const;
    bool menu_down  () const;
    bool menu_left  () const;
    bool menu_right () const;
    bool menu_confirm() const;   // Enter or gamepad A/Start
    bool menu_cancel () const;   // Esc   or gamepad B/Back
    bool menu_pause  () const;   // Esc (in match); gamepad Start is merged in App
    // True on the frame ANY menu input fires (edge). Used by the
    // "press any key" prompt.
    bool menu_any_pressed() const;

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
        const int idx = key_index(key);
        if (idx != kInvalidKeyIndex) current_[idx] = is_down;
    }
    // Mark the current state as the previous frame's state, so the next
    // call to pressed(key) returns true only for keys that are held now
    // but weren't held at this snapshot. Mirrors the SDL_EVENT_KEY_DOWN
    // edge semantics without requiring an actual event.
    void begin_frame_for_test() {
        std::memcpy(previous_, current_, sizeof(previous_));
        quit_requested_ = false;
    }
    // Convenience: clear all held keys (used between scenarios).
    void clear_for_test() {
        std::memset(current_,  0, sizeof(current_));
        std::memset(previous_, 0, sizeof(previous_));
    }

private:
    bool current_[kMaxKeys]{};
    bool previous_[kMaxKeys]{};
    bool quit_requested_{false};
};

} // namespace ember