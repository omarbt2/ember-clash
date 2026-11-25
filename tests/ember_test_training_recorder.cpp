#include "Gameplay/TrainingRecorder.h"

#include <cassert>

using namespace ember;

int main() {
    training::Recorder recorder;
    assert(recorder.size() == 0);
    assert(!recorder.recording());

    recorder.start_recording();
    for (int i = 0; i < 12; ++i) {
        PlayerIntent motion{};
        motion.move_right = (i % 2) == 0;
        motion.jump_pressed = i == 3;
        CombatIntent combat{};
        combat.jab_pressed = i == 5;
        assert(recorder.record(motion, combat));
    }
    recorder.stop_recording();
    assert(recorder.size() == 12);
    assert(!recorder.recording());

    recorder.start_playback();
    for (int i = 0; i < 12; ++i) {
        training::RecordedFrame frame{};
        assert(recorder.next(frame));
        assert(frame.motion.move_right == ((i % 2) == 0));
        assert(frame.motion.jump_pressed == (i == 3));
        assert(frame.combat.jab_pressed == (i == 5));
    }
    training::RecordedFrame end{};
    assert(!recorder.next(end));
    assert(!recorder.playing());

    recorder.start_recording();
    for (std::size_t i = 0; i < training::kMaxRecordedTicks; ++i)
        assert(recorder.record(PlayerIntent{}, CombatIntent{}));
    assert(!recorder.record(PlayerIntent{}, CombatIntent{}));
    assert(recorder.overflowed());
    assert(recorder.size() == training::kMaxRecordedTicks);

    recorder.clear();
    assert(recorder.size() == 0);
    assert(!recorder.overflowed());
    return 0;
}
