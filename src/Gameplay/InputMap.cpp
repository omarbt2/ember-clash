#include "Gameplay/InputMap.h"

#include "Input/InputManager.h"

#include <SDL3/SDL_keycode.h>

namespace ember {

InputMap::Sample InputMap::sample(const InputManager& input, int player_index) {
    Sample s;
    if (player_index == 0) {
        // P1: WASD + Space. Space is an alternate jump key because most
        // platform-fighter muscle memory uses it.
        s.motion.move_left      = input.held(SDLK_A);
        s.motion.move_right     = input.held(SDLK_D);
        s.motion.jump_pressed   = input.pressed(SDLK_SPACE) || input.pressed(SDLK_W);
        s.motion.fast_fall_held = input.held(SDLK_S);

        // M3 attack bindings. J = Jab, K = Forward Tilt, L = Neutral Air.
        // Edge-triggered: InputManager::pressed() reports the rising edge.
        s.combat.jab_pressed   = input.pressed(SDLK_J);
        s.combat.ftilt_pressed = input.pressed(SDLK_K);
        s.combat.nair_pressed  = input.pressed(SDLK_L);
    } else {
        // P2: arrow keys for motion, numpad-1/2/3 for attacks.
        s.motion.move_left      = input.held(SDLK_LEFT);
        s.motion.move_right     = input.held(SDLK_RIGHT);
        s.motion.jump_pressed   = input.pressed(SDLK_UP);
        s.motion.fast_fall_held = input.held(SDLK_DOWN);

        s.combat.jab_pressed   = input.pressed(SDLK_KP_1);
        s.combat.ftilt_pressed = input.pressed(SDLK_KP_2);
        s.combat.nair_pressed  = input.pressed(SDLK_KP_3);
    }
    return s;
}

} // namespace ember
