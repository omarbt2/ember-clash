#pragma once

// M14 gamepad haptics. Translates gameplay events (strong hit, KO,
// menu confirm) into short rumble effects on the attached
// controller. The Haptics layer is intentionally tiny - it has no
// queue, no priority, no concurrent effects - because V1.0 only
// uses rumble as a subtle "feel" cue and we never want rumble to
// be loud enough to be a core gameplay signal.
//
// Design contract:
//
//   1. A call to play_*() MUST never block, never throw, and
//      never crash. It may silently no-op (when the gamepad is
//      detached, when can_rumble() is false, or when the player
//      has disabled haptics in the settings).
//
//   2. Haptics MUST NOT affect simulation. update() is a pure
//      side-effect of "stop the current effect when its time
//      runs out" - it never reads gameplay state.
//
//   3. Haptics MUST NOT affect determinism. The rumble counters
//      used by the test harness live in Gamepad (not in Haptics)
//      so a Haptics instance torn down and reconstructed still
//      sees the same call history. The Haptics state itself
//      (active effect, ticks_remaining) is excluded from
//      state_hash() because it is presentation-only.
//
//   4. The subsystem is owned by App and ticked once per frame
//      from App::step() (after the game logic has run, so the
//      rumble reflects the latest event). When App is shut down,
//      Haptics' destructor stops any in-flight effect so the
//      controller does not keep vibrating after the window
//      closes.

#include "Input/Gamepad.h"

#include <cstdint>

namespace ember {

class Haptics {
public:
    Haptics();
    ~Haptics();

    Haptics(const Haptics&)            = delete;
    Haptics& operator=(const Haptics&) = delete;

    // Bind the live Gamepad so play_*() can route effects.
    // The pointer must outlive the Haptics instance; App
    // enforces this by owning both and destroying Haptics
    // before Gamepad.
    void bind_gamepad(Gamepad* pad) { gamepad_ = pad; }

    // Master switch. When false, every play_*() call is a
    // silent no-op. The flag is read from Settings on App
    // boot and pushed here on kSaveSettings.
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }

    // Effect entry points. All of them are safe to call when
    // the gamepad is missing, when the device has no rumble
    // motors, or when the master switch is off.
    void play_strong_hit();
    void play_ko();
    void play_menu_confirm();
    void play_menu_move();

    // Per-frame tick. Decrements the active-effect timer; when
    // it reaches zero, calls Gamepad::stop_rumble(). This is
    // the only function that calls stop_rumble() during normal
    // operation, and it never cancels an effect early - the
    // intent is that the rumble plays for exactly the duration
    // requested by the most recent play_*() call.
    void update();

    // Stop any in-flight effect immediately. Used on App
    // shutdown so the controller does not keep vibrating after
    // the window is closed.
    void stop_all();

    // -----------------------------------------------------------------
    // Test seam. The headless harness exercises the wiring by
    // calling play_*() and then asserting on Gamepad's test
    // counters (since Haptics itself does not store anything
    // observable - it forwards to Gamepad). We also expose
    // active_effect() / ticks_remaining() so the harness can
    // assert the timer counted down.
    // -----------------------------------------------------------------
    enum class Effect : int {
        kNone = 0,
        kStrongHit,
        kKo,
        kMenuConfirm,
        kMenuMove,
    };
    Effect active_effect()      const { return active_; }
    int    ticks_remaining()    const { return ticks_remaining_; }
    int    effects_played_total() const { return effects_played_total_; }

private:
    // Internal effect start helper. The strength values
    // (low / high) and the duration are mapped from the
    // Effect enum so the tuning lives in one place.
    void start(Effect e);

    Gamepad* gamepad_{nullptr};
    bool     enabled_{true};
    Effect   active_{Effect::kNone};
    int      ticks_remaining_{0};
    // Total count of play_*() calls (regardless of whether
    // they reached the SDL backend). The harness uses this
    // to verify the wiring; it survives effect changes.
    int      effects_played_total_{0};
};

} // namespace ember
