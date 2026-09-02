#include "Gameplay/Attack.h"

namespace ember {

// All values here are starting points per the design brief. The knockback
// formula lives in CombatConfig.h so the entire combat model can be retuned
// without editing this file.
//
// Hitbox origin: fighter-local space, body top-left = (0, 0). For Jab / FTilt
// the hitbox sits in front of the body at roughly fist height; for NAir it
// surrounds the body so it hits any opponent in melee range.
//
// Local space AABBs:
//   Fighter body: 0..48 wide, 0..64 tall
//   Jab:   {12, 24, 28, 16}  -> a 28x16 box from (12, 24) to (40, 40)
//   FTilt: {24,  8, 40, 24}  -> a 40x24 box from (24,  8) to (64, 32), extends past body
//   NAir:  {-8, -8, 64, 80}  -> a 64x80 box from (-8, -8) to (56, 72), surrounds body
//
// Plain integer indices are used for the designated initializers (instead of
// `static_cast<int>(AttackId::Jab)`) because MSVC rejects non-identifier
// designators in array initialization. The static_assert below catches
// accidental reordering of the enum.
static const Attack kAttacks[] = {
    // [0] None: identity. Never starts as an attack; reading it is a bug.
    Attack{
        "None", 0, 0, 0,
        false, false,
        0, 0, 0,
        0.0f, 0.0f,
        {0.0f, 0.0f, 0.0f, 0.0f},
    },
    // [1] Jab: short-ranged, fast, weak. Good for testing single-hit bookkeeping.
    Attack{
        "Jab", 3, 2, 6,
        true, false,
        4, 4, 12,
        0.95f, -0.30f,
        {12.0f, 24.0f, 28.0f, 16.0f},
    },
    // [2] Forward Tilt: longer-ranged, slower, stronger. Pushed at a shallow
    // upward angle so high-% targets fly off the side of the stage.
    Attack{
        "ForwardTilt", 8, 3, 14,
        true, false,
        9, 7, 18,
        0.97f, -0.25f,
        {24.0f, 8.0f, 40.0f, 24.0f},
    },
    // [3] Neutral Air: short air attack, pops the target straight up. Spammable
    // from a double jump to edge-guard / juggle.
    Attack{
        "NeutralAir", 5, 4, 12,
        false, true,
        7, 5, 16,
        0.00f, -1.00f,
        {-8.0f, -8.0f, 64.0f, 80.0f},
    },
};

static_assert(sizeof(kAttacks) / sizeof(kAttacks[0])
              == static_cast<int>(AttackId::Count),
              "kAttacks table is out of sync with AttackId enum");
static_assert(static_cast<int>(AttackId::Jab)         == 1, "AttackId reordered");
static_assert(static_cast<int>(AttackId::ForwardTilt) == 2, "AttackId reordered");
static_assert(static_cast<int>(AttackId::NeutralAir)  == 3, "AttackId reordered");

const Attack& get_attack(AttackId id) {
    const int i = static_cast<int>(id);
    if (i < 0 || i >= static_cast<int>(AttackId::Count)) {
        return kAttacks[0];
    }
    return kAttacks[i];
}

} // namespace ember
