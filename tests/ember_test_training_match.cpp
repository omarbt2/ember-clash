#include "Gameplay/Match.h"
#include "Input/InputManager.h"

#include <SDL3/SDL.h>

#include <cassert>

using namespace ember;

int main() {
    Match match;
    InputManager input;
    match.set_ai(1, nullptr);
    match.force_live_state_for_test();
    input.clear_for_test();

    const float start_x = static_cast<float>(match.fighter(0).body().x);
    match.start_training_recording();
    input.set_held_for_test(SDLK_D, true);
    for (int i = 0; i < 8; ++i) {
        match.update(input);
        input.begin_frame_for_test();
    }
    match.stop_training_recording();
    assert(match.training_recorded_ticks() == 8);
    assert(!match.training_is_recording());

    match.reset();
    match.force_live_state_for_test();
    input.clear_for_test();
    match.play_training_recording();
    const std::uint64_t playback_hash = match.state_hash();
    for (int i = 0; i < 8; ++i) {
        match.update(input);
    }
    assert(match.fighter(0).body().x > start_x);
    assert(!match.training_is_playing());
    assert(match.training_recorded_ticks() == 8);
    assert(match.state_hash() != playback_hash);

    match.clear_training_recording();
    assert(match.training_recorded_ticks() == 0);
    assert(!match.training_is_playing());
    return 0;
}
