#include "Audio/Audio.h"
#include "Core/Logger.h"
#include "Core/Settings.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <random>
#include <string>
namespace ember {
namespace {
// The synthesis helpers below are independent of the Audio
// instance; they live in this anonymous namespace.
// SfxId is an enum class with None + 16 entries (MenuMove through
// Error). Total = 17. The bank_ array has slots for each one.
constexpr int kBankSize = 17;
inline std::int16_t clip16(double v) {
    if (v >  32767.0) return  32767;
    if (v < -32768.0) return -32768;
    return static_cast<std::int16_t>(v);
}
// Apply an attack / decay / release envelope to a synthesized
// waveform. The attack portion ramps from 0 to 1 in attack_s
// seconds; the decay portion ramps from 1 to sustain_level in
// (decay_s - attack_s) seconds; the release portion (if any
// time remains before duration_s) decays linearly from
// sustain_level to 0.
std::vector<std::int16_t> shape(
    const std::vector<double>& raw,
    int sample_rate,
    double attack_s,
    double decay_s,
    double sustain_level)
{
    std::vector<std::int16_t> out(raw.size(), 0);
    const int N = static_cast<int>(raw.size());
    const int attack_n  = std::min(N, std::max(0, static_cast<int>(attack_s * sample_rate)));
    const int decay_n   = std::min(N, std::max(0, static_cast<int>(decay_s  * sample_rate)));
    for (int i = 0; i < N; ++i) {
        double env = 0.0;
        if (i < attack_n) {
            // attack: 0 -> 1
            env = (attack_n == 0) ? 1.0
                                  : static_cast<double>(i) / static_cast<double>(attack_n);
        } else if (i < decay_n) {
            // decay: 1 -> sustain_level
            const double t = (decay_n - attack_n == 0) ? 1.0
                : static_cast<double>(i - attack_n) / static_cast<double>(decay_n - attack_n);
            env = 1.0 + (sustain_level - 1.0) * t;
        } else {
            // release: sustain_level -> 0
            const double t = (N - decay_n == 0) ? 1.0
                : static_cast<double>(i - decay_n) / static_cast<double>(N - decay_n);
            env = sustain_level * (1.0 - t);
        }
        out[i] = clip16(raw[i] * env);
    }
    return out;
}
} // namespace
// ----------------------------------------------------------------------
// Synthesis helpers (member functions; can use kSampleRate)
// ----------------------------------------------------------------------
std::vector<std::int16_t> Audio::synth_sine(
    double freq_hz, double duration_s,
    double attack_s, double decay_s, double sustain_level)
{
    const int N = static_cast<int>(duration_s * kSampleRate);
    std::vector<double> raw(N);
    const double w = 2.0 * 3.14159265358979323846 * freq_hz;
    for (int i = 0; i < N; ++i) {
        raw[i] = std::sin(w * (static_cast<double>(i) / kSampleRate)) * 0.7;
    }
    return shape(raw, kSampleRate, attack_s, decay_s, sustain_level);
}
std::vector<std::int16_t> Audio::synth_square(
    double freq_hz, double duration_s,
    double attack_s, double decay_s, double sustain_level)
{
    const int N = static_cast<int>(duration_s * kSampleRate);
    std::vector<double> raw(N);
    const double period = kSampleRate / freq_hz;
    for (int i = 0; i < N; ++i) {
        const double phase = std::fmod(static_cast<double>(i), period) / period;
        raw[i] = (phase < 0.5 ? 0.6 : -0.6);
    }
    return shape(raw, kSampleRate, attack_s, decay_s, sustain_level);
}
std::vector<std::int16_t> Audio::synth_sawtooth(
    double start_hz, double end_hz, double duration_s,
    double attack_s, double decay_s, double sustain_level)
{
    const int N = static_cast<int>(duration_s * kSampleRate);
    std::vector<double> raw(N);
    double phase = 0.0;
    for (int i = 0; i < N; ++i) {
        const double t  = static_cast<double>(i) / std::max(1, N - 1);
        const double f  = start_hz + (end_hz - start_hz) * t;
        phase += (2.0 * 3.14159265358979323846 * f) / kSampleRate;
        if (phase >  3.14159265358979323846) phase -= 2.0 * 3.14159265358979323846;
        raw[i] = (phase / 3.14159265358979323846) * 0.6;
    }
    return shape(raw, kSampleRate, attack_s, decay_s, sustain_level);
}
std::vector<std::int16_t> Audio::synth_triangle(
    double freq_hz, double duration_s,
    double attack_s, double decay_s, double sustain_level)
{
    const int N = static_cast<int>(duration_s * kSampleRate);
    std::vector<double> raw(N);
    const double period = kSampleRate / freq_hz;
    for (int i = 0; i < N; ++i) {
        const double phase = std::fmod(static_cast<double>(i), period) / period;
        const double tri = (phase < 0.5 ? phase * 2.0 : (1.0 - phase) * 2.0);
        raw[i] = (tri - 0.5) * 0.7;
    }
    return shape(raw, kSampleRate, attack_s, decay_s, sustain_level);
}
std::vector<std::int16_t> Audio::synth_noise(
    double duration_s,
    double attack_s, double decay_s, double sustain_level)
{
    const int N = static_cast<int>(duration_s * kSampleRate);
    std::vector<double> raw(N);
    std::mt19937 rng(0xA0B0C0D0u);
    std::uniform_real_distribution<double> dist(-0.7, 0.7);
    for (int i = 0; i < N; ++i) raw[i] = dist(rng);
    return shape(raw, kSampleRate, attack_s, decay_s, sustain_level);
}
std::vector<std::int16_t> Audio::mix(
    const std::vector<std::int16_t>& a,
    const std::vector<std::int16_t>& b)
{
    const std::size_t N = std::max(a.size(), b.size());
    std::vector<std::int16_t> out(N, 0);
    for (std::size_t i = 0; i < N; ++i) {
        const double av = (i < a.size()) ? a[i] : 0.0;
        const double bv = (i < b.size()) ? b[i] : 0.0;
        out[i] = clip16(av + bv);
    }
    return out;
}
// ----------------------------------------------------------------------
// Bank construction
// ----------------------------------------------------------------------
void Audio::build_bank() {
    // bank_[0] is None; leave empty.
    bank_[static_cast<int>(SfxId::MenuMove)] =
        synth_sine(440.0, 0.050, 0.005, 0.030, 0.0);
    bank_[static_cast<int>(SfxId::MenuConfirm)] =
        mix(synth_sine(660.0, 0.040, 0.005, 0.020, 0.0),
            synth_sine(880.0, 0.040, 0.005, 0.020, 0.0));
    bank_[static_cast<int>(SfxId::MenuCancel)] =
        mix(synth_sine(330.0, 0.030, 0.005, 0.020, 0.0),
            synth_sine(220.0, 0.030, 0.005, 0.020, 0.0));
    bank_[static_cast<int>(SfxId::MenuBack)] =
        synth_sine(220.0, 0.040, 0.005, 0.020, 0.0);
    bank_[static_cast<int>(SfxId::Jump)] =
        synth_sawtooth(220.0, 440.0, 0.200, 0.010, 0.150, 0.0);
    bank_[static_cast<int>(SfxId::DoubleJump)] =
        synth_square(440.0, 0.150, 0.005, 0.120, 0.0);
    // For DoubleJump we wanted a sweep; use sawtooth instead.
    bank_[static_cast<int>(SfxId::DoubleJump)] =
        synth_sawtooth(440.0, 660.0, 0.150, 0.005, 0.120, 0.0);
    bank_[static_cast<int>(SfxId::HitLight)] =
        synth_noise(0.060, 0.005, 0.040, 0.0);
    bank_[static_cast<int>(SfxId::HitHeavy)] =
        mix(synth_noise   (0.150, 0.005, 0.110, 0.0),
            synth_square  (110.0, 0.150, 0.005, 0.110, 0.0));
    bank_[static_cast<int>(SfxId::HitSpark)] =
        synth_triangle(880.0, 0.100, 0.005, 0.080, 0.0);
    bank_[static_cast<int>(SfxId::Ko)] =
        mix(synth_square(110.0, 0.500, 0.020, 0.450, 0.0),
            synth_noise  (0.500, 0.020, 0.450, 0.0));
    // MatchStart: ascending arpeggio (C5, E5, G5, C6) at quarter notes
    // in 300 ms.
    {
        const double step = 0.075;
        auto a = synth_sine(523.25, step, 0.005, step, 0.0);
        auto b = synth_sine(659.25, step, 0.005, step, 0.0);
        auto c = synth_sine(783.99, step, 0.005, step, 0.0);
        auto d = synth_sine(1046.5, step, 0.005, step, 0.0);
        std::vector<std::int16_t> merged;
        merged.reserve(static_cast<std::size_t>(step * 4 * kSampleRate));
        merged.insert(merged.end(), a.begin(), a.end());
        merged.insert(merged.end(), b.begin(), b.end());
        merged.insert(merged.end(), c.begin(), c.end());
        merged.insert(merged.end(), d.begin(), d.end());
        bank_[static_cast<int>(SfxId::MatchStart)] = std::move(merged);
    }
    // Rematch: descending arpeggio.
    {
        const double step = 0.050;
        auto a = synth_sine(1046.5, step, 0.005, step, 0.0);
        auto b = synth_sine(783.99, step, 0.005, step, 0.0);
        auto c = synth_sine(659.25, step, 0.005, step, 0.0);
        auto d = synth_sine(523.25, step, 0.005, step, 0.0);
        std::vector<std::int16_t> merged;
        merged.reserve(static_cast<std::size_t>(step * 4 * kSampleRate));
        merged.insert(merged.end(), a.begin(), a.end());
        merged.insert(merged.end(), b.begin(), b.end());
        merged.insert(merged.end(), c.begin(), c.end());
        merged.insert(merged.end(), d.begin(), d.end());
        bank_[static_cast<int>(SfxId::Rematch)] = std::move(merged);
    }
    bank_[static_cast<int>(SfxId::Pause)]   = synth_sine(220.0, 0.060, 0.005, 0.040, 0.0);
    bank_[static_cast<int>(SfxId::Unpause)] = synth_sine(660.0, 0.060, 0.005, 0.040, 0.0);
    bank_[static_cast<int>(SfxId::LoadingTick)] =
        synth_sine(1000.0, 0.030, 0.002, 0.020, 0.0);
    // Error: dissonant 220 + 233 Hz.
    bank_[static_cast<int>(SfxId::Error)] =
        mix(synth_sine(220.0, 0.200, 0.010, 0.150, 0.0),
            synth_sine(233.0, 0.200, 0.010, 0.150, 0.0));
}
// ----------------------------------------------------------------------
// Lifecycle
// ----------------------------------------------------------------------
Audio::Audio() = default;
Audio::~Audio() {
    shutdown();
}
bool Audio::initialize() {
    if (initialized_) return true;
    // Build the procedural bank first. Even if SDL audio init
    // fails (no HW, headless CI, etc.) the bank is generated
    // and play() can still record diagnostic counts.
    build_bank();
    // Bring the SDL3 audio subsystem up. SDL_InitSubSystem is
    // idempotent within the same process so a caller (App.cpp)
    // that already initialised SDL is safe.
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        Logger::warn("Audio: SDL_InitSubSystem(AUDIO) failed ("
                     + std::string(SDL_GetError() ? SDL_GetError() : "?")
                     + "). Game will run silent.");
        initialized_ = true;
        return true;
    }
    sdl_audio_inited_ = true;
    initialized_ = true;
    SDL_AudioSpec desired{};
    desired.freq     = kSampleRate;
    desired.format   = SDL_AUDIO_S16LE;
    desired.channels = 1;
    SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &desired,
        nullptr,
        nullptr);
    if (stream == nullptr) {
        // No audio HW (headless CI, broken driver). Mark as
        // "no backend" but keep the bank so test_counters
        // still work.
        backend_available_ = false;
        audio_stream_ = nullptr;
        Logger::warn("Audio: SDL_OpenAudioDeviceStream returned NULL ("
                     + std::string(SDL_GetError() ? SDL_GetError() : "?")
                     + "). Game will run silent. play_count() still records.");
        return true;
    }
    audio_stream_ = stream;
    if (!SDL_ResumeAudioStreamDevice(stream)) {
        Logger::warn("Audio: SDL_ResumeAudioStreamDevice failed ("
                     + std::string(SDL_GetError() ? SDL_GetError() : "?")
                     + "). Playback will not advance.");
        // Don't mark backend_available_ false; the device
        // opened, it just won't drain. Caller can still push.
    } else {
        backend_available_ = true;
        Logger::info("Audio: SDL device opened at "
                     + std::to_string(kSampleRate) + " Hz, S16LE, mono.");
    }
    return true;
}
void Audio::shutdown() {
    if (audio_stream_ != nullptr) {
        SDL_DestroyAudioStream(static_cast<SDL_AudioStream*>(audio_stream_));
        audio_stream_ = nullptr;
    }
    if (sdl_audio_inited_) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        sdl_audio_inited_ = false;
    }
    backend_available_ = false;
    initialized_ = false;
}
void Audio::apply_settings(const Settings& s) {
    sfx_mute_     = s.sfx_mute     || s.master_mute;
    music_mute_   = s.music_mute   || s.master_mute;
    sfx_volume_   = s.sfx_volume;
    music_volume_ = s.music_volume;
}
bool Audio::play(SfxId id) {
    const int idx = static_cast<int>(id);
    if (idx < 0 || idx >= kBankSize) return false;
    // Mute check FIRST. When the user mutes SFX (or master),
    // the play() call should be a true no-op: no counter
    // increment, no audio push. This keeps the test harness's
    // 'mute silences play()' contract honest and matches
    // what a real audio library would do (it never sees the
    // call site at all if the user muted the bus).
    if (sfx_mute_) return false;
    if (sfx_volume_ <= 0) return false;
    // Past the mute gate, the request is 'real'; record
    // it so the test harness (and the future diagnostics
    // overlay) can see it.
    play_counts_[idx]++;
    const auto& bank = bank_[idx];
    if (bank.empty()) return false;
    // If no audio device, we still report success: the
    // gameplay code asked for a sound and our internal state
    // (play_counts) is correct. The test seam can override
    // the backend_available_ flag to verify both paths.
    if (audio_stream_ == nullptr) {
        return backend_available_;  // false if CI, true if test forced it
    }
    // Volume-scale into a scratch buffer, then push.
    const double scale = static_cast<double>(sfx_volume_) / 100.0;
    std::vector<std::int16_t> scaled;
    scaled.resize(bank.size());
    for (std::size_t i = 0; i < bank.size(); ++i) {
        scaled[i] = clip16(static_cast<double>(bank[i]) * scale);
    }
    const int len_bytes = static_cast<int>(scaled.size() * sizeof(std::int16_t));
    const bool ok = SDL_PutAudioStreamData(
        static_cast<SDL_AudioStream*>(audio_stream_),
        scaled.data(),
        len_bytes);
    if (!ok) {
        Logger::warn("Audio: SDL_PutAudioStreamData failed ("
                     + std::string(SDL_GetError() ? SDL_GetError() : "?") + ").");
        return false;
    }
    return true;
}
void Audio::music_start(int track_id) {
    if (music_mute_ || music_volume_ <= 0) {
        current_track_ = track_id;
        return;
    }
    current_track_ = track_id;
    Logger::info("Audio: music_start track=" + std::to_string(track_id)
                 + " (V1.0 ships no music assets; this is a no-op)");
}
void Audio::music_stop() {
    if (current_track_ < 0) return;
    current_track_ = -1;
    Logger::info("Audio: music_stop (no music was playing in V1.0)");
}
int Audio::play_count(SfxId id) const {
    const int idx = static_cast<int>(id);
    if (idx < 0 || idx >= 32) return 0;
    return play_counts_[idx];
}
void Audio::reset_counters_for_test() {
    std::memset(play_counts_, 0, sizeof(play_counts_));
}
int Audio::bank_sample_count(SfxId id) const {
    const int idx = static_cast<int>(id);
    if (idx < 0 || idx >= kBankSize) return 0;
    return static_cast<int>(bank_[idx].size());
}
void Audio::set_backend_for_test(bool available) {
    backend_available_ = available;
}
void Audio::set_bank_for_test(SfxId id, std::vector<std::int16_t> samples) {
    const int idx = static_cast<int>(id);
    if (idx < 0 || idx >= kBankSize) return;
    bank_[idx] = std::move(samples);
}
const char* Audio::name_of(SfxId id) {
    switch (id) {
        case SfxId::None:         return "None";
        case SfxId::MenuMove:     return "MenuMove";
        case SfxId::MenuConfirm:  return "MenuConfirm";
        case SfxId::MenuCancel:   return "MenuCancel";
        case SfxId::MenuBack:     return "MenuBack";
        case SfxId::Jump:         return "Jump";
        case SfxId::DoubleJump:   return "DoubleJump";
        case SfxId::HitLight:     return "HitLight";
        case SfxId::HitHeavy:     return "HitHeavy";
        case SfxId::HitSpark:     return "HitSpark";
        case SfxId::Ko:           return "Ko";
        case SfxId::MatchStart:   return "MatchStart";
        case SfxId::Rematch:      return "Rematch";
        case SfxId::Pause:        return "Pause";
        case SfxId::Unpause:      return "Unpause";
        case SfxId::LoadingTick:  return "LoadingTick";
        case SfxId::Error:        return "Error";
    }
    return "Unknown";
}
} // namespace ember


