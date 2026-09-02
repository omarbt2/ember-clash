#include "Gameplay/InputMap.h"

#include "Input/InputManager.h"

#include <SDL3/SDL_keycode.h>

namespace ember {

PlayerIntent InputMap::sample(const InputManager& input, int player_index) {
    PlayerIntent i;
    if (player_index == 0) {
        // P1: WASD + Space. Space is an alternate jump key because most
        // platform-fighter muscle memory uses it.
        i.move_left    = input.held(SDLK_A);
        i.move_right   = input.held(SDLK_D);
        i.jump_pressed = input.pressed(SDLK_SPACE) || input.pressed(SDLK_W);
        i.fast_fall_held = input.held(SDLK_S);
    } else {
        // P2: arrow keys.
        i.move_left    = input.held(SDLK_LEFT);
        i.move_right   = input.held(SDLK_RIGHT);
        i.jump_pressed = input.pressed(SDLK_UP);
        i.fast_fall_held = input.held(SDLK_DOWN);
    }
    return i;
}

} // namespace ember
