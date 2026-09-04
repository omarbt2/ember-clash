#pragma once

#include "Gameplay/Attack.h"
#include "Physics/Motion.h"

#include <SDL3/SDL_keycode.h>

#include <cstddef>
#include <cstdint>

namespace ember {

class InputManager;

// ------------------------------------------------------------------
// KeyBindings
//
// Runtime-mutable key mapping for one player. Stored as SDL_Keycode
// values so they round-trip through Settings persistence as
// integers and remain comparable across runs. The struct is
// trivially copyable (POD) so Settings can hold it directly and
// the remap UI can take a copy on confirm.
//
// Defaults match the V1.0 P1 layout (WASD + Space + J/K/L).
// P2 defaults are intentionally not exposed in V1.0: P2 is the
// AI opponent in the MVP, so its bindings are wired but not
// user-editable. They are kept here so a future 2P local mode
// can rebind P2 without an API change.
// ------------------------------------------------------------------
struct KeyBindings {
    SDL_Keycode move_left{SDLK_A};
    SDL_Keycode move_right{SDLK_D};
    SDL_Keycode jump{SDLK_W};
    SDL_Keycode fast_fall{SDLK_S};
    SDL_Keycode jab{SDLK_J};
    SDL_Keycode ftilt{SDLK_K};
    SDL_Keycode nair{SDLK_L};

    // Equality is value-wise so Settings can spot a real change
    // before flushing. SDL_Keycode is a 32-bit int, so a hand-rolled
    // operator== is the cleanest path.
    bool operator==(const KeyBindings& o) const {
        return move_left  == o.move_left  &&
               move_right == o.move_right &&
               jump       == o.jump       &&
               fast_fall  == o.fast_fall  &&
               jab        == o.jab        &&
               ftilt      == o.ftilt      &&
               nair       == o.nair;
    }
    bool operator!=(const KeyBindings& o) const { return !(*this == o); }

    // Returns a pointer to the slot for the action `index` (0..6)
    // in declaration order, or nullptr if `index` is out of range.
    // The remap UI uses this to look up the active slot when the
    // user is editing a row without a switch in user code.
    SDL_Keycode* slot(int index) {
        switch (index) {
            case 0: return &move_left;
            case 1: return &move_right;
            case 2: return &jump;
            case 3: return &fast_fall;
            case 4: return &jab;
            case 5: return &ftilt;
            case 6: return &nair;
            default: return nullptr;
        }
    }
    const SDL_Keycode* slot(int index) const {
        return const_cast<KeyBindings*>(this)->slot(index);
    }

    // Number of rebindable actions; used to size the remap menu
    // row list and to range-check `slot()`.
    static constexpr int kActionCount = 7;
};

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

    // Active bindings for the two player slots. The default P1
    // bindings are the original hardcoded WASD/SDLK_SPACE/JKL set;
    // P2 is unused in V1.0 (P2 is the AI) but the slot exists so
    // future work can flip P2 to a second human without an API
    // change. The match / menu / app layer reads/writes these to
    // drive the remap UI.
    static KeyBindings& bindings(int player_index);

    // P1-only convenience: returns the P1 bindings reference. The
    // remap UI in Menu talks to this directly; the underlying
    // `bindings(0)` is the same storage.
    static KeyBindings& p1_bindings() { return bindings(0); }

    // Copies the 7 integer keycodes from a Settings::p1_bindings
    // buffer (which is a std::array<int32_t, 7>) into the live
    // P1 KeyBindings. Called by App on init and after every
    // settings save. The signature uses a raw int32_t pointer +
    // count so Settings (Core) does not need to know about
    // KeyBindings (Gameplay) - Core stays a leaf layer.
    static void apply_p1_from_ints(const std::int32_t* keys, std::size_t count);

    // M13.1: same shape as apply_p1_from_ints, but writes the
    // P2 storage. Called by App on init and on every
    // kApplyRemap action that targets a P2 slot. P2 is
    // consumed by the AI in V1.0, but the remap UI persists
    // the bindings so a future 2P local mode does not have
    // to add a settings schema migration.
    static void apply_p2_from_ints(const std::int32_t* keys, std::size_t count);

    // Returns a short, human-readable label for an SDL_Keycode
    // suitable for the remap menu (e.g. SDLK_A -> "A",
    // SDLK_LEFT -> "<-", SDLK_SPACE -> "SPACE"). Writes to
    // `out` (must be at least 8 bytes). The function is
    // pure-C and never allocates, so the menu can call it
    // every frame without touching the allocator. Returns
    // `out` for chaining.
    static const char* key_name(SDL_Keycode key, char* out, std::size_t out_size);
};

} // namespace ember
