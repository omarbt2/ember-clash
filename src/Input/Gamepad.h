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
// This adapter still exposes ONE primary gamepad for menu navigation and
// haptics. InputManager now owns SDL's event queue and forwards hot-plug
// instance IDs here, avoiding the old double-poll bug. P8 still needs a
// multi-slot device manager before 3-4 simultaneous controllers are supported.

#include "Core/InputEdgeState.h"

#include <SDL3/SDL_gamepad.h>

#include <cstdint>

namespace ember {

class InputManager;

class Gamepad {
public:
    Gamepad();
    ~Gamepad();

    Gamepad(const Gamepad&)            = delete;
    Gamepad& operator=(const Gamepad&) = delete;

    // Updates cached device state using the event snapshot produced by the
    // InputManager (the sole SDL event-queue owner). Also discovers a pad
    // already attached before the application started.
    void update(const InputManager& input);

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
    void consume_edges();

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
    // M14 haptics. The gamepad's underlying SDL_Joystick exposes a
    // whole-controller rumble (low + high frequency motors). The
    // can_rumble() query returns false when the device has no
    // rumble motors (e.g. a legacy controller without force-
    // feedback support) or when no gamepad is attached, so
    // callers can short-circuit and never call rumble() on an
    // unsupported device. rumble() itself also short-circuits
    // safely - it is a no-op when can_rumble() is false.
    //
    // SDL_RumbleJoystick automatically stops the rumble after
    // `duration_ms` has elapsed, so we never have to schedule a
    // follow-up stop call. The Haptics subsystem nevertheless
    // tracks an explicit "ticks remaining" counter so the test
    // harness can observe effect lifetimes deterministically
    // without depending on wall-clock time.
    // -----------------------------------------------------------------
    bool can_rumble() const;
    // low/high are 0..65535. duration_ms=0 means "until cancelled".
    void rumble(std::uint16_t low, std::uint16_t high, std::uint32_t duration_ms);
    // Stop any in-flight rumble effect immediately. Safe to call
    // when no gamepad is attached or no rumble is active.
    void stop_rumble();

    // -----------------------------------------------------------------
    // Test seam. The headless harness has no SDL event loop, so it
    // pokes the predicates directly. Production callers should
    // never touch this; the harness uses it to drive the menu nav
    // tests without bringing up an SDL window.
    // -----------------------------------------------------------------
    void set_held_for_test(SDL_GamepadButton b, bool is_down) {
        const int idx = static_cast<int>(b);
        if (idx < 0 || idx >= kMaxButtons) return;
        current_[idx].set_for_test(is_down);
    }
    void poll_button_for_test(SDL_GamepadButton b, bool is_down) {
        const int idx = static_cast<int>(b);
        if (idx >= 0 && idx < kMaxButtons) current_[idx].poll(is_down);
    }
    void begin_frame_for_test() {
        for (auto& button : current_) button.begin_frame_for_test();
    }
    void clear_for_test() {
        for (auto& button : current_) button.reset();
    }
    // Test seam: did the last rumble() call reach the SDL
    // backend? The headless harness can use this to assert that
    // strong-hit / KO events translated into a rumble call
    // without actually exercising the SDL device.
    int  test_rumble_call_count() const { return test_rumble_call_count_; }
    int  test_last_rumble_low()   const { return test_last_rumble_low_; }
    int  test_last_rumble_high()  const { return test_last_rumble_high_; }
    int  test_last_rumble_ms()    const { return test_last_rumble_ms_; }
    int  test_stop_call_count()   const { return test_stop_call_count_; }

    static constexpr int kAxisDeadzone = 8000;  // ~25% of full range
    static constexpr int kMaxButtons   = 32;    // SDL_GAMEPAD_BUTTON_MAX is 21, but we over-allocate for headroom

private:
    SDL_Gamepad* gamepad_{nullptr};
    SDL_JoystickID instance_id_{0};
    char name_[128]{};

    InputEdgeState current_[kMaxButtons]{};

    int  axis_lx_{0};
    int  axis_ly_{0};

    // Test instrumentation: a headless build with no SDL
    // joystick still records the arguments of every rumble
    // call so the harness can assert the Haptics wiring is
    // correct.
    int test_rumble_call_count_{0};
    int test_last_rumble_low_{0};
    int test_last_rumble_high_{0};
    int test_last_rumble_ms_{0};
    int test_stop_call_count_{0};

    void open_device(std::int32_t device_id);
    void close();
};

} // namespace ember
