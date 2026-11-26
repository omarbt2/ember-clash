// Input edge latches must survive render polls until a fixed simulation tick
// consumes them. This exercises the real keyboard and gamepad adapters (with
// their existing headless test seams), not just the shared state helper.

#include "Input/Gamepad.h"
#include "Input/InputManager.h"

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_keycode.h>

#include <cassert>

int main() {
    using namespace ember;

    // A held keyboard key stays held while its one-shot press survives
    // multiple display polls before the next simulation tick.
    InputManager keyboard;
    keyboard.poll_key_for_test(SDLK_J, true);
    keyboard.poll_key_for_test(SDLK_J, true);
    keyboard.poll_key_for_test(SDLK_J, true);
    assert(keyboard.held(SDLK_J));
    assert(keyboard.pressed(SDLK_J));
    assert(!keyboard.released(SDLK_J));
    keyboard.consume_edges();
    assert(keyboard.held(SDLK_J));
    assert(!keyboard.pressed(SDLK_J));

    // A short tap that begins and ends between simulation ticks exposes
    // both edges for one tick, even though the final held state is false.
    keyboard.poll_key_for_test(SDLK_K, true);
    keyboard.poll_key_for_test(SDLK_K, false);
    assert(!keyboard.held(SDLK_K));
    assert(keyboard.pressed(SDLK_K));
    assert(keyboard.released(SDLK_K));
    keyboard.consume_edges();
    assert(!keyboard.pressed(SDLK_K));
    assert(!keyboard.released(SDLK_K));

    // The Gamepad adapter uses the same edge policy as keyboard input.
    Gamepad gamepad;
    gamepad.poll_button_for_test(SDL_GAMEPAD_BUTTON_SOUTH, true);
    gamepad.poll_button_for_test(SDL_GAMEPAD_BUTTON_SOUTH, true);
    assert(gamepad.held(SDL_GAMEPAD_BUTTON_SOUTH));
    assert(gamepad.pressed(SDL_GAMEPAD_BUTTON_SOUTH));
    assert(gamepad.confirm());
    gamepad.consume_edges();
    assert(gamepad.held(SDL_GAMEPAD_BUTTON_SOUTH));
    assert(!gamepad.pressed(SDL_GAMEPAD_BUTTON_SOUTH));

    gamepad.poll_button_for_test(SDL_GAMEPAD_BUTTON_EAST, true);
    gamepad.poll_button_for_test(SDL_GAMEPAD_BUTTON_EAST, false);
    assert(!gamepad.held(SDL_GAMEPAD_BUTTON_EAST));
    assert(gamepad.pressed(SDL_GAMEPAD_BUTTON_EAST));
    assert(gamepad.released(SDL_GAMEPAD_BUTTON_EAST));
    assert(gamepad.cancel());
    gamepad.consume_edges();
    assert(!gamepad.pressed(SDL_GAMEPAD_BUTTON_EAST));
    assert(!gamepad.released(SDL_GAMEPAD_BUTTON_EAST));

    return 0;
}
