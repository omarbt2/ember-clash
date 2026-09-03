#pragma once

// Thin gamepad layer wrapping SDL_Gamepad. The goal is to mirror the
// keyboard API: predicate-style queries (held / pressed / released) so
// the menu and the InputMap can be written identically whether the
// player is on a gamepad or a keyboard.
//
// Today the gamepad layer is used to drive the menu (Up/Down/A/B) and
// to inject motion + attack intents as a fallback when no keyboard is
// present. Wiring the gamepad into Fighter combat is a future slice
// (the InputMap already has a clear seam where the gamepad sample can
// be inserted next to the keyboard sample without touching Fighter).
//
// The layer is a single class, not a full system, because the project
// only needs to support ONE gamepad at a time on the desktop build
// (P1 keyboard / P2 keyboard / P1 gamepad are the three supported
// configurations). A future M9 (local multiplayer) would lift this
// to a vector of gamepads.

#include <SDL3/SDL_gamepad.h>

#include <cstring>

namespace ember {

class Gamepad {
public:
    Gamepad();
    ~Gamepad();

    Gamepad(const Gamepad&)            = delete;
    Gamepad& operator=(const Gamepad&) = delete;

    // Polls SDL events and updates internal state. Call this once
    // per frame from InputManager::update() so the keyboard and
    // gamepad layers stay in lockstep.
    void update();

    // True if a controller is currently attached and open.
    bool is_attached() const { return gamepad_ != nullptr; }

    // The friendly name of the attached controller, or an empty
    // string if none. Used by the controls screen to show the
    // player what is plugged in.
    const char* name() const { return name_[0] ? name_ : ""; }

    // Predicate queries. Buttons are SDL_GamepadButton values.
    bool held   (SDL_GamepadButton b) const;
    bool pressed(SDL_GamepadButton b) const;
    bool released(SDL_GamepadButton b) const;

    // Axis queries. The left stick X axis is in [-32768, 32767]
    // after SDL normalises it; the predicates below apply a small
    // deadzone (kAxisDeadzone) so the stick centre does not cause
    // constant movement.
    float left_stick_x() const;
    float left_stick_y() const;

    // Virtual button predicate: "left" is true when the left stick
    // is pushed past the deadzone to the left, OR when the d-pad
    // left is held. Mirrors InputMap's intent abstraction so the
    // menu nav can use either device.
    bool nav_left  () const;
    bool nav_right () const;
    bool nav_up    () const;
    bool nav_down  () const;
    bool confirm   () const; // A or Start
    bool cancel    () const; // B or Back

    // -----------------------------------------------------------------
    // Test seam. The headless harness has no SDL event loop, so it
    // pokes the predicates directly. Production callers should
    // never touch this; the harness uses it to drive the menu nav
    // tests without bringing up an SDL window.
    // -----------------------------------------------------------------
    void set_held_for_test(SDL_GamepadButton b, bool is_down) {
        const int idx = static_cast<int>(b);
        if (idx < 0 || idx >= kMaxButtons) return;
        current_[idx] = is_down;
    }
    void begin_frame_for_test() {
        std::memcpy(previous_, current_, sizeof(previous_));
    }
    void clear_for_test() {
        std::memset(current_,  0, sizeof(current_));
        std::memset(previous_, 0, sizeof(previous_));
    }

    static constexpr int kAxisDeadzone = 8000;  // ~25% of full range
    static constexpr int kMaxButtons   = 32;    // SDL_GAMEPAD_BUTTON_MAX is 21, but we over-allocate for headroom

private:
    SDL_Gamepad* gamepad_{nullptr};
    SDL_JoystickID instance_id_{0};
    char name_[128]{};

    bool current_ [kMaxButtons]{};
    bool previous_[kMaxButtons]{};

    int  axis_lx_{0};
    int  axis_ly_{0};

    void open_at_index(int index);
    void close();
};

} // namespace ember
