#pragma once

// Audio subsystem. V1.0 ships as a STUB: the API is real, the call
// sites are real, but every play() call is a no-op + an info log.
// This is the right V1.0 posture because:
//
//   1. SDL3 audio playback requires a sound bank (WAV/OGG files).
//      The project has a "no external assets" rule, so the bank
//      would have to be procedurally synthesised - which is a
//      multi-day design effort (sample selection, envelopes,
//      mixer voices, voice pooling).
//   2. The call sites for the audio (hit spark, KO, menu click,
//      pause) are already identified and tagged with the right
//      SFX ID. A future "M12.5-Audio" milestone can drop in the
//      real implementation without touching gameplay code.
//
// Volume is plumbed end-to-end: a play() call respects master /
// sfx / music mute and volume. The stub still returns a
// "started successfully" boolean so the call site can branch on
// it if it ever needs to (e.g. "don't play a hit sound while a
// bigger hit sound is still playing"). For V1.0 the boolean is
// always false because nothing actually plays.

#include "Core/Settings.h"

namespace ember {

// Stable identifiers for every sound the game might play. The
// ID is the contract between the gameplay / UI code and the
// audio implementation; adding a new sound is a one-line enum
// addition + a switch case in Audio::play.
enum class SfxId : int {
    None = 0,
    MenuMove,
    MenuConfirm,
    MenuCancel,
    MenuBack,
    Jump,
    DoubleJump,
    HitLight,
    HitHeavy,
    HitSpark,
    Ko,
    MatchStart,
    Rematch,
    Pause,
    Unpause,
    LoadingTick,
    Error,
};

class Audio {
public:
    Audio();
    ~Audio();

    Audio(const Audio&)            = delete;
    Audio& operator=(const Audio&) = delete;

    // Apply the current settings (volume / mute). Idempotent;
    // the menu calls this whenever the user changes a slider.
    void apply_settings(const Settings& s);

    // Request a sound effect to play. Stub: logs the call,
    // respects mute flags, returns false (nothing actually
    // played). Returns true only when audio would actually
    // be audible: not muted and the master / sfx / music
    // family is above zero.
    bool play(SfxId id);

    // Music start / stop. Stub: no-op + log. The parameter is
    // a logical track ID so the future real implementation
    // can decide which sample to play. For now there are no
    // real tracks; passing any value is fine.
    void music_start(int track_id);
    void music_stop();

    // Returns the number of times a sound was requested
    // since the last reset_counters_for_test() call. Used
    // by the M12 test harness to verify the right sound
    // fired without depending on any audio output.
    int  play_count(SfxId id) const;
    void reset_counters_for_test();

    // One-time initialisation (called from App::initialize).
    // Stub: logs that audio is disabled. The future real
    // implementation will open the SDL audio device and
    // allocate a mixer voice pool here.
    void initialize();

    // Returns true if audio playback is actually available.
    // For V1.0 this is always false; the future real
    // implementation flips it to true once the device opens.
    bool backend_available() const { return false; }

    // Convenience: returns a human-readable name for a SfxId.
    // Used by the diagnostics overlay and the test harness.
    static const char* name_of(SfxId id);

private:
    bool sfx_mute_{false};
    bool music_mute_{false};
    int  sfx_volume_{80};
    int  music_volume_{60};
    int  play_counts_[32]{};   // indexed by static_cast<int>(SfxId)
    int  current_track_{-1};
};

} // namespace ember
