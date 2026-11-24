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
    // ---- Rebirth Phase 2 ------------------------------------------------
    // [4] Up Tilt: anti-air poke.
    Attack{ "UpTilt", 6, 3, 13, true, false, 7, 6, 16, 0.20f, -1.00f,
            {8.0f, -24.0f, 32.0f, 30.0f} },
    // [5] Down Tilt: low poke that trips grounded targets into the air.
    Attack{ "DownTilt", 5, 2, 12, true, false, 6, 5, 14, 0.60f, -0.80f,
            {16.0f, 44.0f, 40.0f, 20.0f} },
    // [6] Forward Smash: slow, committal, the main killer sideways.
    Attack{ "ForwardSmash", 16, 4, 26, true, false, 16, 12, 28, 0.90f, -0.45f,
            {20.0f, 6.0f, 52.0f, 34.0f} },
    // [7] Up Smash: vertical killer.
    Attack{ "UpSmash", 14, 5, 28, true, false, 14, 11, 26, 0.10f, -1.00f,
            {-4.0f, -30.0f, 56.0f, 44.0f} },
    // [8] Down Smash: wide, hits both sides.
    Attack{ "DownSmash", 12, 6, 30, true, false, 13, 10, 24, 0.70f, -0.70f,
            {-16.0f, 38.0f, 80.0f, 26.0f} },
    // [9] Forward Air.
    Attack{ "ForwardAir", 9, 4, 16, false, true, 9, 8, 18, 0.80f, -0.60f,
            {20.0f, 10.0f, 40.0f, 32.0f} },
    // [10] Back Air: hits behind, launches backwards.
    Attack{ "BackAir", 7, 3, 14, false, true, 10, 8, 18, -0.90f, -0.40f,
            {-34.0f, 12.0f, 40.0f, 30.0f} },
    // [11] Up Air: juggle tool.
    Attack{ "UpAir", 6, 4, 14, false, true, 6, 6, 14, 0.00f, -1.00f,
            {2.0f, -26.0f, 44.0f, 32.0f} },
    // [12] Down Air: spike (launches downwards), laggy.
    Attack{ "DownAir", 12, 4, 20, false, true, 8, 7, 18, 0.30f, 1.00f,
            {8.0f, 50.0f, 32.0f, 34.0f} },
    // [13] Neutral Special - Ember Burst: wide blast, both grounded and air.
    Attack{ "EmberBurst", 10, 5, 22, false, false, 8, 7, 18, 0.90f, -0.40f,
            {-20.0f, -6.0f, 88.0f, 76.0f} },
    // [14] Side Special - Flare Dash: lunges forward.
    Attack{ "FlareDash", 6, 8, 20, false, false, 8, 7, 16, 0.90f, -0.40f,
            {16.0f, 12.0f, 40.0f, 36.0f}, 9.0f, 0.0f, false },
    // [15] Up Special - Rising Phoenix: recovery, once per airtime.
    Attack{ "RisingPhoenix", 5, 8, 24, false, false, 7, 6, 14, 0.10f, -1.00f,
            {4.0f, -20.0f, 40.0f, 60.0f}, 0.0f, -13.5f, true },
    // [16] Down Special - Ground Slam: dive + shockwave.
    Attack{ "GroundSlam", 14, 5, 26, false, false, 11, 9, 22, 0.60f, -0.80f,
            {-24.0f, 36.0f, 96.0f, 30.0f}, 0.0f, 12.0f, false },
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
