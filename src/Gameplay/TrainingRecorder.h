#pragma once

#include "Physics/Motion.h"
#include "Gameplay/Attack.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace ember::training {

// Bounded, deterministic input capture for the first Training Mode slice.
// It stores player intents (not SDL events), so recordings are portable
// across keyboard/gamepad devices and safe to feed back through Match.
inline constexpr std::size_t kMaxRecordedTicks = 600;

struct RecordedFrame {
    PlayerIntent motion{};
    CombatIntent combat{};
};

class Recorder {
public:
    bool start_recording() {
        count_ = 0;
        cursor_ = 0;
        recording_ = true;
        playing_ = false;
        overflowed_ = false;
        return true;
    }

    bool record(const PlayerIntent& motion, const CombatIntent& combat) {
        if (!recording_) return false;
        if (count_ >= frames_.size()) {
            recording_ = false;
            overflowed_ = true;
            return false;
        }
        frames_[count_++] = RecordedFrame{motion, combat};
        return true;
    }

    void stop_recording() { recording_ = false; cursor_ = 0; }

    void start_playback() {
        recording_ = false;
        playing_ = count_ > 0;
        cursor_ = 0;
    }

    bool next(RecordedFrame& out) {
        if (!playing_ || cursor_ >= count_) {
            playing_ = false;
            return false;
        }
        out = frames_[cursor_++];
        if (cursor_ >= count_) playing_ = false;
        return true;
    }

    void clear() {
        count_ = cursor_ = 0;
        recording_ = playing_ = overflowed_ = false;
    }

    std::size_t size() const { return count_; }
    std::size_t cursor() const { return cursor_; }
    bool recording() const { return recording_; }
    bool playing() const { return playing_; }
    bool overflowed() const { return overflowed_; }
    const RecordedFrame* frame_at(std::size_t index) const {
        return index < count_ ? &frames_[index] : nullptr;
    }

private:
    std::array<RecordedFrame, kMaxRecordedTicks> frames_{};
    std::size_t count_{0};
    std::size_t cursor_{0};
    bool recording_{false};
    bool playing_{false};
    bool overflowed_{false};
};

} // namespace ember::training
