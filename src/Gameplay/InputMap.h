#pragma once

#include "Gameplay/Attack.h"
#include "Physics/Motion.h"

namespace ember {

class InputManager;

// Translates raw keyboard state into platform-agnostic player intents. This
// is the ONLY translation unit that may reference SDLK_* (apart from the
// InputManager itself). Gamepad and touch layers will plug in alongside
// this file in later milestones without touching Fighter or Arena.
class InputMap {
public:
    // Samples the current input state for the given player and returns
    // motion + combat intents suitable for feeding Fighter::update().
    // Motion and combat are returned separately because they are decoded
    // at different boundaries (Physics for motion, Gameplay for combat),
    // but both come from the same keyboard snapshot per frame.
    struct Sample {
        PlayerIntent motion;
        CombatIntent combat;
    };
    static Sample sample(const InputManager& input, int player_index);
};

} // namespace ember
