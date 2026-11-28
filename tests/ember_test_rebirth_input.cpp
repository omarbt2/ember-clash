// -----------------------------------------------------------------------------
// Rebirth input-to-gameplay test.
//
// The recovery snapshot had fighter logic for specials, smashes, guard, dodge
// and Ember Surge, but nothing in InputMap/Settings could reach it. This test
// drives the REAL path: InputManager (synthetic keys) -> InputMap::sample ->
// Match::update -> Fighter state, for both players as humans.
//
// Covers: 13-key layout parity (KeyBindings vs Settings defaults), settings
// persistence incl. legacy 7-key files, per-key intent mapping, player
// isolation, rebinding, and every new move family reaching Fighter state.
// -----------------------------------------------------------------------------
#include "Core/Settings.h"
#include "Core/Time.h"
#include "Gameplay/AiController.h"
#include "Gameplay/CombatConfig.h"
#include "Gameplay/InputMap.h"
#include "Gameplay/Match.h"
#include "Input/InputManager.h"

#include <SDL3/SDL_keycode.h>

#include <cstdio>
#include <fstream>
#include <string>

using namespace ember;

static int g_pass = 0, g_fail = 0;
static void check(bool ok, const char* what) {
    std::printf("  %s  %s\n", ok ? "PASS" : "FAIL", what);
    ok ? ++g_pass : ++g_fail;
}

static void tick(Match& m, InputManager& in, int n = 1) {
    for (int i = 0; i < n; ++i) { m.update(in); in.begin_frame_for_test(); }
}

// Fresh two-human match with both fighters standing on the ground, far apart.
struct Rig {
    Match m;
    InputManager in;
    Rig() {
        Settings d; d.reset_to_defaults();
        InputMap::apply_p1_from_ints(d.p1_bindings.data(), d.p1_bindings.size());
        InputMap::apply_p2_from_ints(d.p2_bindings.data(), d.p2_bindings.size());
        m.force_live_state_for_test();
        m.set_ai(1, nullptr);
        tick(m, in, 180);
    }
};

int main() {
    std::printf("[rebirth-input] starting\n");
    Settings d; d.reset_to_defaults();

    // 0) Regression: keycodes >= 512 (arrows, keypad, shift...) must not alias.
    {
        const SDL_Keycode ks[] = {SDLK_LEFT, SDLK_RIGHT, SDLK_UP, SDLK_DOWN, SDLK_KP_0, SDLK_KP_1,
                                  SDLK_KP_4, SDLK_KP_8, SDLK_LSHIFT, SDLK_RSHIFT, SDLK_LCTRL,
                                  SDLK_A, SDLK_SPACE, SDLK_RETURN, SDLK_ESCAPE};
        const int n = static_cast<int>(sizeof(ks) / sizeof(ks[0]));
        bool distinct = true, roundtrip = true;
        for (int i = 0; i < n; ++i) {
            roundtrip = roundtrip && InputManager::key_from_index(InputManager::key_index(ks[i])) == ks[i];
            for (int j = i + 1; j < n; ++j)
                if (InputManager::key_index(ks[i]) == InputManager::key_index(ks[j])) distinct = false;
        }
        check(distinct, "arrows/keypad/shift/ctrl each get their own slot (no aliasing)");
        check(roundtrip, "key_from_index(key_index(k)) == k (remap capture returns the real key)");
        check(InputManager::is_valid_keycode(SDLK_LEFT) &&
              InputManager::is_valid_keycode(SDLK_LSHIFT) &&
              InputManager::is_valid_keycode(SDLK_RETURN) &&
              InputManager::is_valid_keycode(SDLK_ESCAPE) &&
              !InputManager::is_valid_keycode(SDLK_UNKNOWN) &&
              !InputManager::is_valid_keycode(static_cast<SDL_Keycode>(5)) &&
              !InputManager::is_valid_keycode(static_cast<SDL_Keycode>(127)) &&
              !InputManager::is_valid_keycode(static_cast<SDL_Keycode>(159)) &&
              !InputManager::is_valid_keycode(static_cast<SDL_Keycode>(0x7fffffff)),
              "valid extended keycodes are accepted and unknown/corrupt codes are rejected");
        check(InputManager::key_index(SDLK_UNKNOWN) == InputManager::kInvalidKeyIndex &&
              InputManager::key_from_index(InputManager::kInvalidKeyIndex) == 0,
              "invalid-key sentinel cannot alias a real scancode slot");
        InputManager menu_keys;
        menu_keys.set_held_for_test(SDLK_P, true);
        check(!menu_keys.menu_pause(), "P remains a gameplay key and does not pause");
        menu_keys.clear_for_test();
        menu_keys.set_held_for_test(SDLK_ESCAPE, true);
        check(menu_keys.menu_pause() && menu_keys.menu_cancel(),
              "Escape pauses a match and remains the menu cancel key");
        InputManager im;
        im.set_held_for_test(SDLK_LEFT, true);
        check(im.held(SDLK_LEFT) && !im.held(SDLK_RIGHT) && !im.held(SDLK_UP) && !im.held(SDLK_KP_1),
              "holding Left does not read as Right/Up/Keypad1");

        // any_held is a level query, not an event-this-frame query.
        im.clear_for_test();
        im.set_held_for_test(SDLK_A, true);
        check(im.any_held(), "any_held is true immediately after a key is pressed");
        im.begin_frame_for_test();
        check(im.any_held(), "any_held stays true across frames while a key remains held");
        im.set_held_for_test(SDLK_D, true);
        im.set_held_for_test(SDLK_A, false);
        check(im.any_held(), "releasing one key keeps any_held true while another remains held");
        im.set_held_for_test(SDLK_D, false);
        check(!im.any_held(), "any_held becomes false only after every tracked key is released");
    }

    // 1) Settings defaults must equal KeyBindings defaults, slot by slot.
    {
        KeyBindings p1;  // P1 defaults
        bool ok1 = true;
        for (int i = 0; i < KeyBindings::kActionCount; ++i)
            ok1 = ok1 && (static_cast<std::int32_t>(*p1.slot(i)) == d.p1_bindings[i]);
        check(ok1, "Settings P1 defaults == KeyBindings defaults (all 13)");
        const KeyBindings& p2 = InputMap::bindings(1);
        Rig r;  // ensures live P2 = Settings P2 defaults
        bool ok2 = true;
        for (int i = 0; i < KeyBindings::kActionCount; ++i)
            ok2 = ok2 && (static_cast<std::int32_t>(*p2.slot(i)) == d.p2_bindings[i]);
        check(ok2, "live P2 bindings == Settings P2 defaults (all 13)");
        bool unique = true;
        for (int i = 0; i < 13; ++i) for (int j = i + 1; j < 13; ++j) {
            if (d.p1_bindings[i] == d.p1_bindings[j]) unique = false;
            if (d.p2_bindings[i] == d.p2_bindings[j]) unique = false;
        }
        check(unique, "no duplicate keys within a player's 13 defaults");
        bool disjoint = true;
        for (int i = 0; i < 13; ++i) for (int j = 0; j < 13; ++j)
            if (d.p1_bindings[i] == d.p2_bindings[j]) disjoint = false;
        check(disjoint, "P1 and P2 default keys are disjoint");
    }

    // 2) Persistence: 13 keys round-trip; a legacy 7-key file keeps new defaults.
    {
        Settings s; s.reset_to_defaults();
        s.p1_bindings[8]  = SDLK_Q;
        s.p2_bindings[12] = SDLK_Z;
        const std::string path = "ember_test_rebirth_input.ini";
        check(save_settings_to_file(path, s), "settings saved");
        const Settings r = load_settings_from_file(path);
        check(r.p1_bindings[8] == SDLK_Q && r.p2_bindings[12] == SDLK_Z,
              "new-slot bindings round-trip through save/load");
        check(r.p1_bindings == s.p1_bindings && r.p2_bindings == s.p2_bindings,
              "all 26 bindings round-trip exactly");
        std::ofstream(path, std::ios::trunc)
            << "p1_special=2147483647\n" << "p2_dodge=-5\n";
        const Settings sanitized = load_settings_from_file(path);
        check(sanitized.p1_bindings[8] == d.p1_bindings[8] &&
              sanitized.p2_bindings[11] == d.p2_bindings[11],
              "corrupt keycodes restore the affected action defaults");
        std::ofstream(path) << "p1_move_left=97\np1_jab=106\n";   // legacy-style file
        const Settings l = load_settings_from_file(path);
        check(l.p1_bindings[8] == d.p1_bindings[8] && l.p1_bindings[12] == d.p1_bindings[12],
              "legacy file without new keys keeps the new-key defaults");
        std::remove(path.c_str());
    }

    // 3) Per-key intent mapping + player isolation via the real InputMap::sample.
    {
        Rig r;
        const KeyBindings p1 = InputMap::bindings(0), p2 = InputMap::bindings(1);
        r.in.clear_for_test();
        r.in.set_held_for_test(p1.special, true);
        r.in.set_held_for_test(p1.up, true);
        r.in.set_held_for_test(p1.shield, true);
        r.in.set_held_for_test(p1.smash, true);
        r.in.set_held_for_test(p1.dodge, true);
        r.in.set_held_for_test(p1.surge, true);
        auto a = InputMap::sample(r.in, 0);
        check(a.combat.special_pressed && a.motion.up_held && a.combat.shield_held &&
              a.combat.smash_held && a.combat.dodge_pressed && a.combat.surge_pressed,
              "P1 keys produce all six new intents");
        auto b = InputMap::sample(r.in, 1);
        check(!b.combat.special_pressed && !b.motion.up_held && !b.combat.shield_held &&
              !b.combat.smash_held && !b.combat.dodge_pressed && !b.combat.surge_pressed,
              "P1 keys leak nothing into P2's sample");
        r.in.clear_for_test();
        r.in.set_held_for_test(p2.special, true);
        auto c = InputMap::sample(r.in, 0);
        auto e = InputMap::sample(r.in, 1);
        check(!c.combat.special_pressed && e.combat.special_pressed,
              "P2 special key reaches only P2");
        r.in.clear_for_test();
        r.in.set_held_for_test(p1.shield, true);
        r.in.begin_frame_for_test();           // now held-but-not-new
        check(InputMap::sample(r.in, 0).combat.shield_held, "shield is level-triggered (held)");
        r.in.set_held_for_test(p1.dodge, true);
        r.in.begin_frame_for_test();
        check(!InputMap::sample(r.in, 0).combat.dodge_pressed, "dodge is edge-triggered (not repeated while held)");
    }

    // 4) Real gameplay: each move family reachable from keys, per fighter state.
    {
        Rig r;
        const KeyBindings p1 = InputMap::bindings(0);
        Fighter& f = r.m.fighter(0);
        check(f.is_grounded() && f.state() == Fighter::State::Idle, "rig: P1 grounded and idle");

        r.in.clear_for_test(); r.in.set_held_for_test(p1.special, true);
        tick(r.m, r.in);
        check(f.state() == Fighter::State::Attacking && f.current_attack_id() == AttackId::NeutralSpecial,
              "special key -> NeutralSpecial");
        r.in.clear_for_test(); tick(r.m, r.in, 120);

        r.in.set_held_for_test(p1.up, true); r.in.set_held_for_test(p1.jab, true);
        tick(r.m, r.in);
        check(f.current_attack_id() == AttackId::UpTilt, "up + jab -> UpTilt");
        r.in.clear_for_test(); tick(r.m, r.in, 120);

        r.in.set_held_for_test(p1.move_right, true);
        r.in.set_held_for_test(p1.smash, true); r.in.set_held_for_test(p1.ftilt, true);
        tick(r.m, r.in);
        check(f.current_attack_id() == AttackId::ForwardSmash, "smash modifier + forward + strong -> ForwardSmash");
        r.in.clear_for_test(); tick(r.m, r.in, 120);


        // Guard: level-triggered, enters Guard on the ground.
        r.in.set_held_for_test(p1.shield, true);
        tick(r.m, r.in, 2);
        check(f.state() == Fighter::State::Guard, "shield key held -> Guard state");
        r.in.clear_for_test(); tick(r.m, r.in, 60);
        check(f.state() != Fighter::State::Guard, "releasing shield leaves Guard");

        // Dodge: edge press from neutral.
        r.in.set_held_for_test(p1.dodge, true);
        tick(r.m, r.in);
        check(f.state() == Fighter::State::Dodge, "dodge key -> Dodge state");
        r.in.clear_for_test(); tick(r.m, r.in, 120);

        // Ember Surge: only activates with a full meter.
        r.in.set_held_for_test(p1.surge, true); tick(r.m, r.in);
        check(!f.surge_active(), "surge key with empty meter does nothing");
        r.in.clear_for_test(); tick(r.m, r.in, 2);
        f.set_surge_meter_for_test(combat::kSurgeMeterMax);
        r.in.set_held_for_test(p1.surge, true); tick(r.m, r.in);
        check(f.surge_active(), "surge key with full meter activates Ember Surge");
    }

    // 5) Two humans at once: simultaneous input, no cross-talk.
    {
        Rig r;
        const KeyBindings p1 = InputMap::bindings(0), p2 = InputMap::bindings(1);
        r.in.clear_for_test();
        r.in.set_held_for_test(p1.shield, true);
        r.in.set_held_for_test(p2.jab, true);
        tick(r.m, r.in);
        check(r.m.fighter(0).state() == Fighter::State::Guard &&
              r.m.fighter(1).state() == Fighter::State::Attacking &&
              r.m.fighter(1).current_attack_id() == AttackId::Jab,
              "P1 guards while P2 jabs in the same frame");
        r.in.clear_for_test(); tick(r.m, r.in, 120);
        r.in.set_held_for_test(p2.special, true);
        tick(r.m, r.in);
        check(r.m.fighter(1).current_attack_id() == AttackId::NeutralSpecial &&
              r.m.fighter(0).state() == Fighter::State::Idle,
              "P2 special does not disturb P1");
    }

    // 6) Rebinding changes behaviour: old key dead, new key live.
    {
        Rig r;
        std::int32_t keys[13];
        for (int i = 0; i < 13; ++i) keys[i] = static_cast<std::int32_t>(*InputMap::bindings(0).slot(i));
        const SDL_Keycode old_key = InputMap::bindings(0).special;
        keys[8] = SDLK_Q;
        InputMap::apply_p1_from_ints(keys, 13);
        r.in.clear_for_test(); r.in.set_held_for_test(old_key, true);
        tick(r.m, r.in);
        check(r.m.fighter(0).state() == Fighter::State::Idle, "old special key is dead after rebind");
        r.in.clear_for_test(); tick(r.m, r.in, 2);
        r.in.set_held_for_test(SDLK_Q, true);
        tick(r.m, r.in);
        check(r.m.fighter(0).current_attack_id() == AttackId::NeutralSpecial, "new special key works after rebind");
    }

    std::printf("\n=== REBIRTH-INPUT: %d pass, %d fail ===\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
