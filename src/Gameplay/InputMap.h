#pragma once

#include "Physics/Motion.h"

namespace ember {

class InputManager;

// Translates raw keyboard state into platform-agnostic player intents. This
// is the ONLY translation unit that may reference SDLK_* (apart from the
// InputManager itself). Gamepad and touch layers will plug in alongside
// this file in later milestones without touching Fighter or Arena.
class InputMap {
public:
    // Samples the current input state for the given player and returns an
    // intent suitable for feeding Fighter::update().
    static PlayerIntent sample(const InputManager& input, int player_index);
};

} // namespace ember
