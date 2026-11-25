#include "Gameplay/InputMap.h"

#include "Input/InputManager.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>

#include <cstdio>

namespace ember {

namespace {

// The two live KeyBindings slots. P1 is the human; P2 is currently
// used by the AI opponent in the first release, but its full 13-key
// layout is persisted and exposed in Settings for the future local-2P
// mode. The storage
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
        b.up         = SDLK_KP_8;
        b.special    = SDLK_KP_4;
        b.smash      = SDLK_KP_0;
        b.shield     = SDLK_KP_5;
        b.dodge      = SDLK_KP_6;
        b.surge      = SDLK_KP_7;
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
    // Copy only as many slots as the caller supplied (older 7-key
    // callers keep the defaults for the six newer actions).
    const std::size_t n = (count < static_cast<std::size_t>(KeyBindings::kActionCount))
                        ? count
                        : static_cast<std::size_t>(KeyBindings::kActionCount);
    for (std::size_t i = 0; i < n; ++i) {
        *b.slot(static_cast<int>(i)) = static_cast<SDL_Keycode>(keys[i]);
    }
}

void InputMap::apply_p2_from_ints(const std::int32_t* keys, std::size_t count) {
    if (!keys) return;
    KeyBindings& b = bindings_storage(1);
    // Copy only as many slots as the caller supplied (older 7-key
    // callers keep the defaults for the six newer actions).
    const std::size_t n = (count < static_cast<std::size_t>(KeyBindings::kActionCount))
                        ? count
                        : static_cast<std::size_t>(KeyBindings::kActionCount);
    for (std::size_t i = 0; i < n; ++i) {
        *b.slot(static_cast<int>(i)) = static_cast<SDL_Keycode>(keys[i]);
    }
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
    s.motion.move_left      = input.held(b.move_left);
    s.motion.move_right     = input.held(b.move_right);
    s.motion.jump_pressed   = input.pressed(b.jump);
    // P1 keeps the SPACE escape hatch (space is common platformer
    // muscle memory) unless the user bound jump to space itself.
    if (player_index == 0 && b.jump != SDLK_SPACE && input.pressed(SDLK_SPACE)) {
        s.motion.jump_pressed = true;
    }
    s.motion.fast_fall_held = input.held(b.fast_fall);
    s.motion.up_held        = input.held(b.up);

    s.combat.jab_pressed     = input.pressed(b.jab);
    s.combat.ftilt_pressed   = input.pressed(b.ftilt);
    s.combat.nair_pressed    = input.pressed(b.nair);
    s.combat.special_pressed = input.pressed(b.special);
    s.combat.smash_held      = input.held(b.smash);
    s.combat.shield_held     = input.held(b.shield);
    s.combat.dodge_pressed   = input.pressed(b.dodge);
    s.combat.surge_pressed   = input.pressed(b.surge);
    return s;
}

} // namespace ember
