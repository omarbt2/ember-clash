#include "Input/Gamepad.h"

#include "Core/Logger.h"
#include "Input/InputManager.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>

#include <algorithm>
#include <cstring>
#include <string>

namespace ember {

Gamepad::Gamepad() {
    // Initialise the gamepad subsystem. Safe to call multiple times
    // (SDL_InitSubSystem is reference-counted).
    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
        Logger::warn(std::string{"Gamepad: SDL_InitSubSystem failed: "}
                     + SDL_GetError());
    }
}

Gamepad::~Gamepad() {
    close();
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
}

void Gamepad::open_device(std::int32_t device_id) {
    if (gamepad_) close();
    SDL_Gamepad* pad = SDL_OpenGamepad(static_cast<SDL_JoystickID>(device_id));
    if (!pad) {
        Logger::warn(std::string{"Gamepad: SDL_OpenGamepad("}
                     + std::to_string(device_id) + ") failed: " + SDL_GetError());
        return;
    }
    gamepad_    = pad;
    instance_id_ = SDL_GetGamepadID(pad);
    const char* n = SDL_GetGamepadName(pad);
    if (n) {
        const size_t len = std::min<std::size_t>(std::strlen(n), sizeof(name_) - 1);
        std::memcpy(name_, n, len);
        name_[len] = '\0';
    } else {
        name_[0] = '\0';
    }
    Logger::info(std::string{"Gamepad attached: "} + name_);
}

void Gamepad::close() {
    if (gamepad_) {
        Logger::info(std::string{"Gamepad detached: "} + name_);
        SDL_CloseGamepad(gamepad_);
        gamepad_ = nullptr;
        instance_id_ = 0;
        name_[0] = '\0';
        std::memset(current_,  0, sizeof(current_));
        std::memset(previous_, 0, sizeof(previous_));
        axis_lx_ = 0;
        axis_ly_ = 0;
    }
}

void Gamepad::update(const InputManager& input) {
    std::memcpy(previous_, current_, sizeof(previous_));
    std::memset(current_, 0, sizeof(current_));
    axis_lx_ = 0;
    axis_ly_ = 0;

    // Process detach events before attach events so a controller replaced in
    // one poll frame cannot leave the old handle selected.
    for (int i = 0; i < input.gamepad_removed_count(); ++i) {
        if (gamepad_ && input.gamepad_removed_id(i) == instance_id_) close();
    }
    if (!gamepad_) {
        for (int i = 0; i < input.gamepad_added_count(); ++i) {
            open_device(input.gamepad_added_id(i));
            if (gamepad_) break;
        }
    }

    // Also discover controllers that were connected before SDL's first event
    // poll. SDL_GetGamepads returns device IDs, not transient array positions.
    if (!gamepad_) {
        int count = 0;
        SDL_JoystickID* ids = SDL_GetGamepads(&count);
        if (ids) {
            for (int i = 0; i < count && !gamepad_; ++i) {
                open_device(static_cast<std::int32_t>(ids[i]));
            }
            SDL_free(ids);
        }
    }

    if (!gamepad_) return;
    for (int i = 0; i < kMaxButtons; ++i) {
        current_[i] = SDL_GetGamepadButton(gamepad_, static_cast<SDL_GamepadButton>(i));
    }
    axis_lx_ = SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_LEFTX);
    axis_ly_ = SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_LEFTY);
}

bool Gamepad::held(SDL_GamepadButton b) const {
    const int i = static_cast<int>(b);
    if (i < 0 || i >= kMaxButtons) return false;
    return current_[i];
}
bool Gamepad::pressed(SDL_GamepadButton b) const {
    const int i = static_cast<int>(b);
    if (i < 0 || i >= kMaxButtons) return false;
    return current_[i] && !previous_[i];
}
bool Gamepad::released(SDL_GamepadButton b) const {
    const int i = static_cast<int>(b);
    if (i < 0 || i >= kMaxButtons) return false;
    return !current_[i] && previous_[i];
}

float Gamepad::left_stick_x() const {
    constexpr float kMax = 32767.0f;
    if (axis_lx_ >  kAxisDeadzone) return  (axis_lx_ - kAxisDeadzone) / (kMax - kAxisDeadzone);
    if (axis_lx_ < -kAxisDeadzone) return  (axis_lx_ + kAxisDeadzone) / (kMax - kAxisDeadzone);
    return 0.0f;
}
float Gamepad::left_stick_y() const {
    constexpr float kMax = 32767.0f;
    if (axis_ly_ >  kAxisDeadzone) return  (axis_ly_ - kAxisDeadzone) / (kMax - kAxisDeadzone);
    if (axis_ly_ < -kAxisDeadzone) return  (axis_ly_ + kAxisDeadzone) / (kMax - kAxisDeadzone);
    return 0.0f;
}

bool Gamepad::nav_left  () const { return current_[SDL_GAMEPAD_BUTTON_DPAD_LEFT]  || left_stick_x() < -0.5f; }
bool Gamepad::nav_right () const { return current_[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] || left_stick_x() >  0.5f; }
bool Gamepad::nav_up    () const { return current_[SDL_GAMEPAD_BUTTON_DPAD_UP]    || left_stick_y() < -0.5f; }
bool Gamepad::nav_down  () const { return current_[SDL_GAMEPAD_BUTTON_DPAD_DOWN]  || left_stick_y() >  0.5f; }
bool Gamepad::confirm   () const { return pressed(SDL_GAMEPAD_BUTTON_SOUTH) || pressed(SDL_GAMEPAD_BUTTON_START); }
bool Gamepad::cancel    () const { return pressed(SDL_GAMEPAD_BUTTON_EAST)  || pressed(SDL_GAMEPAD_BUTTON_BACK); }

bool Gamepad::can_rumble() const {
    if (!gamepad_) return false;
    SDL_Joystick* joy = SDL_GetGamepadJoystick(gamepad_);
    if (!joy) return false;
    SDL_PropertiesID props = SDL_GetJoystickProperties(joy);
    if (!props) return false;
    return SDL_GetBooleanProperty(props, SDL_PROP_JOYSTICK_CAP_RUMBLE_BOOLEAN, false);
}

void Gamepad::rumble(std::uint16_t low, std::uint16_t high, std::uint32_t duration_ms) {
    // Record the call for the test harness even when there is
    // no gamepad. The headless build never has a real device,
    // so the SDL_RumbleJoystick branch is skipped; the counter
    // is still incremented so tests can verify the call
    // happened.
    ++test_rumble_call_count_;
    test_last_rumble_low_  = low;
    test_last_rumble_high_ = high;
    test_last_rumble_ms_   = static_cast<int>(duration_ms);

    if (!can_rumble()) return;
    SDL_Joystick* joy = SDL_GetGamepadJoystick(gamepad_);
    // Duration is the SDL_RumbleJoystick-internal stop time.
    // SDL also stops the rumble automatically when duration_ms
    // has elapsed; callers do not need to schedule a follow-up
    // stop call. We pass the duration verbatim.
    if (!SDL_RumbleJoystick(joy, low, high, duration_ms)) {
        Logger::warn(std::string{"Gamepad: SDL_RumbleJoystick failed: "}
                     + SDL_GetError());
    }
}

void Gamepad::stop_rumble() {
    ++test_stop_call_count_;
    if (!can_rumble()) return;
    SDL_Joystick* joy = SDL_GetGamepadJoystick(gamepad_);
    if (!SDL_RumbleJoystick(joy, 0, 0, 0)) {
        Logger::warn(std::string{"Gamepad: SDL_RumbleJoystick(stop) failed: "}
                     + SDL_GetError());
    }
}

} // namespace ember
