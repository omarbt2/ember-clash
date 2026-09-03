#pragma once

// Main menu, pause menu, and rematch prompt.
//
// One Menu instance owns the whole UI state machine. App drives it
// with menu_update(input) every frame and menu_render(renderer) once
// per frame. The Menu never touches Match or Fighter directly - it
// only emits Action events ("start match", "quit", "settings
// changed", etc.) that App translates into calls on the gameplay
// systems.
//
// State machine:
//
//   kTitle     -> splash + big logo + "Press any key" hint
//   kMain      -> top-level menu (Play / Settings / Controls / Quit)
//   kPlay      -> the in-match UI is owned by Match/HUD; Menu is
//                 dormant (update returns kNone every frame) until
//                 the app pushes an event into it.
//   kPaused    -> pause overlay (Resume / Settings / Restart / Quit
//                 to Title)
//   kRematch   -> post-KO choice (Rematch / Back to Title)
//   kSettings  -> settings screen (audio + display + AI difficulty)
//   kControls  -> controls screen (read-only list of bindings)
//   kQuitConfirm -> "Are you sure?" -> Yes / Cancel
//
// The transition graph is the union of all "Action" outcomes. The
// App is the one that actually executes the side effects (start a
// match, save settings, etc); the Menu just decides what to do
// next.

#include "Core/Settings.h"
#include "Graphics/Renderer.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ember {

class Gamepad;
class InputManager;
class Audio;

class Menu {
public:
    enum class State {
        kTitle,
        kMain,
        kPlay,
        kPaused,
        kRematch,
        kSettings,
        kControls,
        kQuitConfirm,
    };

    // The Menu emits one of these each frame (or kNone if the
    // current state has no user-driven action pending). App reads
    // it from consume_action() and decides what to do.
    enum class Action {
        kNone,
        kStartMatch,
        kReturnToTitle,
        kRestartMatch,
        kResumeMatch,
        kSaveSettings,
        kRestoreDefaultSettings,
        kRequestQuit,
        kToggleFullscreen,
        kApplyRenderScale,
    };

    // Events the App can push into the menu.
    enum class Event {
        kMatchStarted,    // -> kPlay
        kMatchEnded,      // -> kRematch
        kMatchPaused,     // -> kPaused
        kMatchResumed,    // -> kPlay
    };

    // A single menu entry. Label is a human-readable string
    // rendered with the bitmap font; Value is an optional right-
    // aligned value (used for "Master volume: 80"); OnConfirm
    // is the action this entry commits to when selected +
    // confirmed.
    struct Entry {
        std::string label;
        std::string value;       // empty for a plain action
        Action      on_confirm;
    };

    Menu();

    // Bind the live settings so the Settings screen can edit them
    // in place. The menu stores a pointer (not a copy) so changes
    // are immediately observable to the rest of the game; the App
    // is responsible for flushing the file on save.
    void bind_settings(Settings* s) { settings_ = s; }

    // Push an external event into the menu state machine. Used
    // by App to inform the menu that a match ended (so the menu
    // can pop into the rematch prompt) or that the match was
    // paused (so the menu can show the pause overlay).
    void push_event(Event ev);

    // Driven by App each frame. The menu reads the input +
    // gamepad, advances its state, and updates the entry
    // selection / value editor. Returns the current State so
    // App can decide what to render / simulate.
    State update(const InputManager& input, const Gamepad& pad, Audio& audio);

    // Render the current screen. For kPlay the menu is dormant
    // and the function does nothing - the App renders Match.
    void render(Renderer& renderer) const;

    // Read the most recent pending action and clear it. The
    // menu emits at most one action per frame.
    Action consume_action();

    State state() const { return state_; }

    // True when the menu is "active" (i.e. the App should NOT
    // be running Match). For kPlay the answer is false - the
    // App should be running Match and rendering the game.
    bool blocks_simulation() const;

    // Force the menu to a particular state. Used by App to jump
    // to kTitle on the kReturnToTitle action, and by the test
    // harness to drive the state machine without an SDL window.
    void set_state(State s) { state_ = s; selected_index_ = 0; }

    // -----------------------------------------------------------------
    // Test seam. Mirrors the M6-C / M11 input pattern. Production
    // callers never touch this; the harness uses it to drive the
    // menu nav and the settings editor without a window.
    // -----------------------------------------------------------------
    void set_state_for_test(State s) { set_state(s); }
    int  selected_index_for_test() const { return selected_index_; }
    void set_settings_for_test(Settings* s) { settings_ = s; }

private:
    // State-local helpers.
    void render_title    (Renderer& r) const;
    void render_main     (Renderer& r) const;
    void render_paused   (Renderer& r) const;
    void render_rematch  (Renderer& r) const;
    void render_settings (Renderer& r) const;
    void render_controls (Renderer& r) const;
    void render_quit_confirm(Renderer& r) const;

    // Entry list for the current state. Recomputed on every
    // state entry and on every settings edit so the right-hand
    // value column always reflects the live settings struct.
    const std::vector<Entry>& entries_for_state(State s) const;
    void rebuild_settings_entries();

    // Returns true if `ev` is consumed (only when the menu is
    // actually listening for an external event, e.g. MatchOver).
    bool handle_event(Event ev);

    // Common navigation logic shared by all menu states. Updates
    // selected_index_ based on up/down/confirm/cancel and returns
    // the resulting Action (or kNone).
    Action navigate(const InputManager& input, const Gamepad& pad, Audio& audio);

    State state_{State::kTitle};
    int   selected_index_{0};
    Action pending_action_{Action::kNone};

    // Settings editing: which field is currently being edited by
    // the +/- keys. -1 means "no field being edited" (the
    // highlighted entry is treated as a plain button).
    int  settings_focus_{-1};

    // Settings entries are recomputed whenever the user touches
    // a value so the right-hand column is always current. They
    // are stored as a member (not a const-ref to a function
    // local) so the navigation code can hold a pointer.
    std::vector<Entry> settings_entries_;
    // The other states' entries are static, so we cache them
    // here to avoid a per-frame allocation.
    std::vector<Entry> main_entries_;
    std::vector<Entry> paused_entries_;
    std::vector<Entry> rematch_entries_;
    std::vector<Entry> controls_entries_;

    // Pointer to the live settings (owned by App). The menu
    // never owns a copy.
    Settings* settings_{nullptr};

    // Cached button-press detection. The menu's navigate() reads
    // menu_up / menu_down from the input manager, which are
    // already edge-triggered; no extra debounce is needed.

    // Tick counter for the title-screen pulse animation.
    int  title_pulse_ticks_{0};

    // True when we're inside the settings "value editor" sub-
    // state, where left/right adjust the value instead of
    // navigating the list.
    bool editing_value_{false};
};

} // namespace ember
