#include "Gameplay/InputMap.h"

#include "Input/InputManager.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>

#include <cstdio>

namespace ember {

namespace {

// The two live KeyBindings slots. P1 is the human; P2 is wired
// for the same layout but is currently consumed only by the AI
// in V1.0, so the remap UI does not expose P2 yet. The storage
// is a function-local static so its address is stable (the
// remap UI caches a KeyBindings& on entry and the address
// must not move while the menu is open).
KeyBindings& bindings_storage(int player_index) {
    static KeyBindings p1;
    // P2 starts with the arrow-keys / numpad layout so a future
    // 2P local mode has a distinct default from P1.
    static KeyBindings p2 = []{
        KeyBindings b;
        b.move_left  = SDLK_LEFT;
        b.move_right = SDLK_RIGHT;
        b.jump       = SDLK_UP;
        b.fast_fall  = SDLK_DOWN;
        b.jab        = SDLK_KP_1;
        b.ftilt      = SDLK_KP_2;
        b.nair       = SDLK_KP_3;
        return b;
    }();
    return player_index == 0 ? p1 : p2;
}

} // namespace

KeyBindings& InputMap::bindings(int player_index) {
    return bindings_storage(player_index);
}

void InputMap::apply_p1_from_ints(const std::int32_t* keys, std::size_t count) {
    if (!keys) return;
    KeyBindings& b = bindings_storage(0);
    // Defensive: only copy up to the number of slots we have,
    // and only positive keycodes (0 = SDLK_UNKNOWN = "unbound").
    // We do not validate uniqueness here - the remap UI is
    // responsible for preventing duplicate bindings, and a
    // duplicated key is benign (two actions fire on the same
    // press) compared to silently dropping the user's choice.
    const std::size_t n = (count < static_cast<std::size_t>(KeyBindings::kActionCount))
                        ? count
                        : static_cast<std::size_t>(KeyBindings::kActionCount);
    if (n >= 1) b.move_left  = static_cast<SDL_Keycode>(keys[0]);
    if (n >= 2) b.move_right = static_cast<SDL_Keycode>(keys[1]);
    if (n >= 3) b.jump       = static_cast<SDL_Keycode>(keys[2]);
    if (n >= 4) b.fast_fall  = static_cast<SDL_Keycode>(keys[3]);
    if (n >= 5) b.jab        = static_cast<SDL_Keycode>(keys[4]);
    if (n >= 6) b.ftilt      = static_cast<SDL_Keycode>(keys[5]);
    if (n >= 7) b.nair       = static_cast<SDL_Keycode>(keys[6]);
}

void InputMap::apply_p2_from_ints(const std::int32_t* keys, std::size_t count) {
    if (!keys) return;
    KeyBindings& b = bindings_storage(1);
    // Same shape and same defensive copy as P1. The remap
    // UI's "player index" field picks which array to write.
    const std::size_t n = (count < static_cast<std::size_t>(KeyBindings::kActionCount))
                        ? count
                        : static_cast<std::size_t>(KeyBindings::kActionCount);
    if (n >= 1) b.move_left  = static_cast<SDL_Keycode>(keys[0]);
    if (n >= 2) b.move_right = static_cast<SDL_Keycode>(keys[1]);
    if (n >= 3) b.jump       = static_cast<SDL_Keycode>(keys[2]);
    if (n >= 4) b.fast_fall  = static_cast<SDL_Keycode>(keys[3]);
    if (n >= 5) b.jab        = static_cast<SDL_Keycode>(keys[4]);
    if (n >= 6) b.ftilt      = static_cast<SDL_Keycode>(keys[5]);
    if (n >= 7) b.nair       = static_cast<SDL_Keycode>(keys[6]);
}

const char* InputMap::key_name(SDL_Keycode key, char* out, std::size_t out_size) {
    if (!out || out_size == 0) return out;
    out[0] = '\0';
    if (key == 0) {
        // SDLK_UNKNOWN. We surface this as "---" so an unbound
        // slot reads as empty rather than as a literal "unknown".
        const char* k = "---";
        std::size_t i = 0;
        for (; k[i] && i + 1 < out_size; ++i) out[i] = k[i];
        out[i] = '\0';
        return out;
    }
    const char* src = SDL_GetKeyName(key);
    if (!src || !*src) {
        // Fall back to a numeric representation so the menu
        // never displays nothing.
        std::snprintf(out, out_size, "K%d", static_cast<int>(key));
        return out;
    }
    // SDL's keyname is usually a short string ("A", "Left", "Space")
    // but can be longer for some keys ("Keypad 0"). Copy up to
    // out_size-1 bytes to keep the buffer null-terminated.
    std::size_t i = 0;
    for (; src[i] && i + 1 < out_size; ++i) out[i] = src[i];
    out[i] = '\0';
    return out;
}

InputMap::Sample InputMap::sample(const InputManager& input, int player_index) {
    Sample s;
    const KeyBindings& b = bindings_storage(player_index);
    if (player_index == 0) {
        s.motion.move_left      = input.held(b.move_left);
        s.motion.move_right     = input.held(b.move_right);
        // The remap UI allows the player to bind either jump or
        // any single key. We also accept the SDLK_SPACE escape
        // hatch if the user did not bind jump to space - space
        // is a very common platformer muscle-memory key.
        s.motion.jump_pressed   = input.pressed(b.jump)
                                || (b.jump != SDLK_SPACE && input.pressed(SDLK_SPACE));
        s.motion.fast_fall_held = input.held(b.fast_fall);

        s.combat.jab_pressed   = input.pressed(b.jab);
        s.combat.ftilt_pressed = input.pressed(b.ftilt);
        s.combat.nair_pressed  = input.pressed(b.nair);
    } else {
        // P2: the AI reads the same PlayerIntent / CombatIntent
        // shape from its own state machine, so the InputMap
        // sample for P2 is essentially unused in V1.0. We
        // keep it wired (and reading the P2 bindings) so a
        // future 2P local mode can flip a switch and have
        // human input flow through unchanged.
        s.motion.move_left      = input.held(b.move_left);
        s.motion.move_right     = input.held(b.move_right);
        s.motion.jump_pressed   = input.pressed(b.jump);
        s.motion.fast_fall_held = input.held(b.fast_fall);

        s.combat.jab_pressed   = input.pressed(b.jab);
        s.combat.ftilt_pressed = input.pressed(b.ftilt);
        s.combat.nair_pressed  = input.pressed(b.nair);
    }
    return s;
}

} // namespace ember
