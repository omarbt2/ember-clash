#include "Audio/Audio.h"

#include "Core/Logger.h"

#include <cstring>
#include <string>

namespace ember {

Audio::Audio() = default;
Audio::~Audio() = default;

void Audio::initialize() {
    // V1.0 STUB. The real implementation will:
    //   1. SDL_InitSubSystem(SDL_INIT_AUDIO)
    //   2. SDL_OpenAudioDevice with the desired spec
    //   3. Allocate a voice pool (N simultaneous SFX voices, 1 music)
    //   4. Load the procedural sample bank (WAV files generated at
    //      boot, or kept in-memory as synthesised waveforms).
    Logger::info("Audio: V1.0 stub - no audio playback. "
                 "All play() calls are no-ops (logged for diagnostics).");
}

void Audio::apply_settings(const Settings& s) {
    sfx_mute_     = s.sfx_mute     || s.master_mute;
    music_mute_   = s.music_mute   || s.master_mute;
    sfx_volume_   = s.sfx_volume;
    music_volume_ = s.music_volume;
}

bool Audio::play(SfxId id) {
    const int idx = static_cast<int>(id);
    if (idx < 0 || idx >= 32) return false;
    if (sfx_mute_ || sfx_volume_ <= 0) {
        // Don't even log in release builds - every keypress in
        // a menu would spam the log. The future real audio
        // implementation will route through a debug overlay
        // instead.
        return false;
    }
    ++play_counts_[idx];
    Logger::info(std::string{"Audio: play "} + name_of(id)
                 + " (stub; would have been audible)");
    return false;  // nothing actually played
}

void Audio::music_start(int track_id) {
    if (music_mute_ || music_volume_ <= 0) return;
    current_track_ = track_id;
    Logger::info("Audio: music_start track=" + std::to_string(track_id)
                 + " (stub)");
}

void Audio::music_stop() {
    if (current_track_ < 0) return;
    Logger::info("Audio: music_stop (stub)");
    current_track_ = -1;
}

int Audio::play_count(SfxId id) const {
    const int idx = static_cast<int>(id);
    if (idx < 0 || idx >= 32) return 0;
    return play_counts_[idx];
}

void Audio::reset_counters_for_test() {
    std::memset(play_counts_, 0, sizeof(play_counts_));
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
