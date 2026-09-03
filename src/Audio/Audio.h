#pragma once
// Audio subsystem.
//
// The V1.0 implementation is a real SDL3 audio backend with a
// procedurally-synthesised sample bank. There are no external
// audio assets: every sound effect is generated at init() time
// from a short synthesis recipe (sine, square, triangle, noise)
// and an attack-decay envelope. The bank is stored in-memory
// and pushed to an SDL_AudioStream on play().
//
// Why procedural?
//   * The project has a "no external assets" rule. A real sound
//     bank would mean shipping WAV/OGG files, which contradicts
//     that rule.
//   * The sound effects needed for V1.0 (a hit, a KO, a menu
//     click) are short, low-frequency, and not high-fidelity.
//     Procedural synthesis hits the bar with ~50 lines of code
//     per effect.
//   * No asset loading code, no decoding path, no file-IO
//     surface in the audio module.
//
// What lives in the bank (per SfxId):
//   * MenuMove:        short click (440 Hz sine, 50 ms)
//   * MenuConfirm:     rising two-tone (660 -> 880 Hz, 80 ms)
//   * MenuCancel:      falling two-tone (330 -> 220 Hz, 60 ms)
//   * MenuBack:        single note (220 Hz, 40 ms)
//   * Jump:            sawtooth sweep (220 -> 440 Hz, 200 ms)
//   * DoubleJump:      square sweep (440 -> 660 Hz, 150 ms)
//   * HitLight:        noise burst (60 ms)
//   * HitHeavy:        noise + 110 Hz square (150 ms)
//   * HitSpark:        triangle 880 Hz decay (100 ms)
//   * Ko:              110 Hz square + noise, slow decay (500 ms)
//   * MatchStart:      ascending arpeggio (300 ms)
//   * Rematch:         descending arpeggio (200 ms)
//   * Pause:           220 Hz single note (60 ms)
//   * Unpause:         660 Hz single note (60 ms)
//   * LoadingTick:     click (1000 Hz, 30 ms)
//   * Error:           dissonant 220 + 233 Hz (200 ms)
//
// All samples are 22050 Hz, mono, int16. ~22 KB per sound at
// 500 ms worst case; total bank is well under 200 KB.
//
// Threading model: play() is called from the main game thread
// and pushes samples into the SDL_AudioStream. SDL_AudioStream
// is internally thread-safe for Put; the audio device callback
// runs on SDL's audio thread. No mutex needed here.
//
// Backend failure: if SDL_OpenAudioDeviceStream returns NULL
// (no audio HW, no driver, etc.), the Audio class still works:
// bank generation happens, play() still records the call in
// play_count(), the only difference is that no samples are
// actually pushed. This keeps the game playable in a CI /
// headless environment where no audio device exists.
#include <cstdint>
#include <vector>
namespace ember {

// Forward declaration. The full Settings type is only needed in
// Audio.cpp apply_settings(); keeping the include out of the header
// avoids a Settings.h -> <array> dependency on every translation unit
// that includes Audio.h.
struct Settings;
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
    // Open the SDL3 audio device and generate the procedural
    // sample bank. Returns true on full success (device + bank);
    // returns true even on partial success (bank generated but
    // device open failed) so the gameplay code can still call
    // play() for the diagnostics. Returns false only on a
    // catastrophic failure (SDL audio subsystem unavailable).
    bool initialize();
    // Tear down the audio device. Safe to call multiple times.
    // Idempotent.
    void shutdown();
    // Apply the current settings (volume / mute). Idempotent;
    // the menu calls this whenever the user changes a slider.
    void apply_settings(const Settings& s);
    // Request a sound effect to play. Returns true iff the sound
    // would be audible (not muted, sample bank populated). If
    // the SDL audio device opened, also pushes the sample buffer
    // (volume-scaled) into the stream.
    bool play(SfxId id);
    // Music start / stop. The parameter is a logical track ID;
    // the V1.0 implementation has a single silent music track
    // (we ship no music assets), so music_start logs and
    // increments a counter; music_stop logs. The interface is
    // real so a future music-loaded milestone can drop in the
    // real implementation.
    void music_start(int track_id);
    void music_stop();
    // Returns the number of times a sound was requested since
    // the last reset_counters_for_test() call. Used by the M12
    // test harness to verify the right sound fired without
    // depending on any audio output.
    int  play_count(SfxId id) const;
    void reset_counters_for_test();
    // Returns true iff the SDL audio device actually opened and
    // samples will be audible. False in headless / CI / no-HW
    // environments; the gameplay code uses this to suppress
    // "audio backend" diagnostics.
    bool backend_available() const { return backend_available_; }
    // Returns the number of samples (int16) in the procedurally
    // generated bank for a given SfxId. Used by the test
    // harness to assert the bank was generated.
    int  bank_sample_count(SfxId id) const;
    // Test seam: override the backend-available flag. Lets the
    // harness assert both "real device present" and "no device"
    // code paths without actually opening / closing the OS
    // audio device. Production code never calls this.
    void set_backend_for_test(bool available);
    // Test seam: pre-fill a bank entry with caller-supplied
    // samples. The real bank is procedurally generated; this
    // hook lets the test verify the play()-and-push path with a
    // known signature (e.g. all-zeroes, all-max). Production
    // code never calls this.
    void set_bank_for_test(SfxId id, std::vector<std::int16_t> samples);
    // Convenience: returns a human-readable name for a SfxId.
    static const char* name_of(SfxId id);
private:
    // Synthesis helpers. All return int16 mono samples at
    // 22050 Hz, with the given envelope already applied.
    static std::vector<std::int16_t> synth_sine(
        double freq_hz, double duration_s,
        double attack_s, double decay_s, double sustain_level);
    static std::vector<std::int16_t> synth_square(
        double freq_hz, double duration_s,
        double attack_s, double decay_s, double sustain_level);
    static std::vector<std::int16_t> synth_sawtooth(
        double start_hz, double end_hz, double duration_s,
        double attack_s, double decay_s, double sustain_level);
    static std::vector<std::int16_t> synth_triangle(
        double freq_hz, double duration_s,
        double attack_s, double decay_s, double sustain_level);
    static std::vector<std::int16_t> synth_noise(
        double duration_s,
        double attack_s, double decay_s, double sustain_level);
    // Mix several sources together (sum, clipped). Returns
    // a fresh vector sized to the longest input (zero-pads
    // the shorter ones).
    static std::vector<std::int16_t> mix(
        const std::vector<std::int16_t>& a,
        const std::vector<std::int16_t>& b);
    // Build the entire bank at init.
    void build_bank();
    // Mute / volume. Cached from the last apply_settings() call.
    bool sfx_mute_{false};
    bool music_mute_{false};
    int  sfx_volume_{80};
    int  music_volume_{60};
    // Per-SfxId play count. Indexed by static_cast<int>(SfxId).
    int  play_counts_[32]{};
    int  current_track_{-1};
    // Bank storage. bank_[0] is the None slot and is always empty.
    std::vector<std::int16_t> bank_[17];
    // SDL3 audio. Opaque pointer; we never dereference it in the
    // header. The forward declaration is in the .cpp via
    // SDL3/SDL_audio.h, not here, so the header does not pull
    // in SDL3 transitively.
    void*                audio_stream_{nullptr};
    bool                 sdl_audio_inited_{false};
    bool                 backend_available_{false};
    bool                 initialized_{false};
    static constexpr int kSampleRate = 22050;
};
} // namespace ember


