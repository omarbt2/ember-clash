#pragma once

// Persistent user preferences. Loaded from a small INI-style file on
// startup, written back on change. The file format is line-based
// "key=value" so it is human-readable and trivially portable between
// desktop and (future) Android / web builds.
//
// All fields are plain-old-data so the struct is trivially copyable
// and safe to read from a const reference. Mutating the settings
// (via a SettingsMenu action) marks the struct dirty; the App calls
// save_if_dirty() once per second on the main thread to flush the
// file. This avoids hammering the filesystem on every menu click.

#include <array>
#include <cstdint>
#include <string>

namespace ember {

struct Settings {
    // Volume sliders are 0-100 inclusive. Mute is a separate flag so
    // the user can re-enable without losing the slider position.
    int  master_volume{80};
    int  sfx_volume{80};
    int  music_volume{60};
    bool master_mute{false};
    bool sfx_mute{false};
    bool music_mute{false};

    // Display.
    bool fullscreen{false};
    // Internal render scale 1-2 (1 = native 1280x720, 2 = 2560x1440
    // logical with the renderer scaling down). The renderer does not
    // yet honour this fully; the value is stored so the settings
    // screen can read/write it without crashing, and so a future
    // renderer patch has a knob to bind to.
    int  render_scale{1};

    // Accessibility.
    //
    // screen_shake:
    //   When true (default), the match applies a post-KO screen
    //   shake. When false, the shake amount is forced to 0. The
    //   setting is honoured in App::render(): the view offset is
    //   passed through only when screen_shake is true. Useful for
    //   players with vestibular sensitivity.
    //
    // color_blind_palette:
    //   When true, fighters render with a high-contrast palette
    //   (the implementation is a stub: the setting is stored and
    //   exposed to the renderer via get_color_blind_palette() so
    //   future palette-swap work has a knob to bind to). The HUD
    //   also swaps damage-counter ink to a yellow/orange scheme
    //   for better contrast against the background.
    bool screen_shake{true};
    bool color_blind_palette{false};

    // Input. P1 and P2 each have a control scheme slot. The strings
    // are stable identifiers; rebinding screen will be a later
    // feature. Today the only valid value is "keyboard" for either
    // slot. The slots are kept as fixed-size strings to keep the
    // struct trivially copyable for the snapshot copy in the menu.
    std::array<char, 16> p1_scheme{"keyboard"};
    std::array<char, 16> p2_scheme{"keyboard"};

    // AI difficulty preset name: "easy" / "standard" / "hard".
    // Matched against ai::kEasy / ai::kStandard / ai::kHard in AiConfig.h.
    std::array<char, 16> ai_difficulty{"standard"};

    // P1 key bindings. Stored as raw 32-bit integers (SDL_Keycode
    // values) so Settings does not need to pull in the SDL header.
    // The order matches KeyBindings in Gameplay/InputMap.h:
    //   0 = move_left, 1 = move_right, 2 = jump, 3 = fast_fall,
    //   4 = jab, 5 = ftilt, 6 = nair.
    // The defaults are the original WASD + Space + JKL set; an
    // out-of-range integer (e.g. 0 = SDLK_UNKNOWN) is replaced with
    // the corresponding default at load time so a corrupt file
    // cannot leave the player with no input.
    std::array<std::int32_t, 7> p1_bindings{0};

    // P2 key bindings. M13.1: same shape as P1, defaults are the
    // arrow-keys + numpad layout (distinct from P1's WASD+JKL).
    // P2 is consumed by the AI in V1.0, but the rebind UI is
    // shared between P1 and P2, so we persist both sets. A future
    // 2P local mode (M9+) flips a switch and these are read by
    // InputMap as-is.
    std::array<std::int32_t, 7> p2_bindings{0};

    // Reset everything to compile-time defaults. Used by the
    // "Restore defaults" button on the settings screen and by the
    // unit tests.
    void reset_to_defaults();

    // Equality / inequality. The struct is plain-old-data so a
    // hand-rolled operator== is the cleanest path; std::array
    // already provides operator==.
    bool operator==(const Settings& other) const;
    bool operator!=(const Settings& other) const { return !(*this == other); }
};

// Loads the settings file from `path`. If the file does not exist
// or is unreadable, returns a default-constructed Settings. Parse
// errors on individual lines are silently skipped (the line is
// dropped and the field keeps its default) so a corrupted file
// never blocks the user from reaching the main menu.
Settings load_settings_from_file(const std::string& path);

// Writes the settings to `path` using a line-based format. Returns
// true on success. A best-effort atomic write (write to .tmp then
// rename) is used so a crash mid-write cannot leave a half-file
// that would fail to parse on the next launch.
bool save_settings_to_file(const std::string& path, const Settings& s);

// Path helper. On desktop the file lives next to the executable
// (in production it would go in the per-user app data directory;
// we keep it beside the .exe for V1.0 simplicity). On Emscripten
// the path is ignored and the settings live in localStorage; the
// load/save functions in this file are desktop-only stubs. The
// path is exposed as a function so a test can override it.
std::string default_settings_path();

} // namespace ember
