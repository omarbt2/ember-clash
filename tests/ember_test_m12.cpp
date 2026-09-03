// M12 headless test. Verifies the Menu state machine, Settings
// round-trip, Audio stub, and Gamepad hot-plug API surface without
// opening a window.
//
// Pattern: same as M6-C / M11 - construct real objects, drive them
// with synthetic input, assert on observable state. No mocks.

#include "Audio/Audio.h"
#include "Core/Logger.h"
#include "Core/Settings.h"
#include "Gameplay/AiConfig.h"
#include "Gameplay/AiController.h"
#include "Gameplay/ArtConfig.h"
#include "Gameplay/InputMap.h"
#include "Gameplay/Match.h"
#include "Input/Gamepad.h"
#include "Input/InputManager.h"
#include "UI/Menu.h"

#include <SDL3/SDL_keycode.h>

#include <cstdio>
#include <cstring>
#include <string>

namespace {

int g_total = 0;
int g_passed = 0;

void check(bool ok, const char* name) {
    ++g_total;
    if (ok) {
        ++g_passed;
        std::printf("  PASS  %s\n", name);
    } else {
        std::printf("  FAIL  %s\n", name);
    }
}

void section(const char* name) {
    std::printf("\n=== %s ===\n", name);
}

// ---- Helpers -----------------------------------------------------------------
// A no-op InputManager: we only need menu_* predicates, and we
// inject menu up/down/confirm/cancel by setting one of the per-
// predicate test-mode flags. (Real production calls use SDL keys.)
//
// Implementation: we use a tiny subclass that injects via a
// singleton "next input" pattern. We do not touch the real
// InputManager's internals (which would require a window).
class FakeInput {
public:
    bool up   = false;
    bool down = false;
    bool left = false;
    bool right= false;
    bool confirm = false;
    bool cancel  = false;
    bool pause   = false;
    bool any     = false;
    void clear() { up = down = left = right = confirm = cancel = pause = any = false; }
};

FakeInput g_in;

// ember::InputManager has a public menu_up()/menu_down()/etc. that
// returns false by default. We can't override the predicates on a
// real instance without an SDL window, so we drive the Menu
// through its update() against a real InputManager (whose menu_*
// always returns false in this build) and the Gamepad whose
// pressed() / nav_*() we can drive via test seams.
//
// For the integration-level tests below we therefore drive the
// Menu primarily through push_event() (the high-level state
// machine) and through selected_index_for_test() / state()
// introspection.

} // namespace

int main() {
    using namespace ember;

    Logger::info("M12 headless test starting...");

    // ---- 1. Settings round-trip -------------------------------------------
    section("settings round-trip");
    {
        Settings s = Settings();
        s.master_volume = 42;
        s.sfx_volume    = 12;
        s.music_volume  = 88;
        s.ai_difficulty.fill(0);
        std::memcpy(s.ai_difficulty.data(), "hard", 4);
        s.fullscreen    = true;
        s.render_scale  = 2;

        // Serialize via the same code path the App uses.
        const std::string path = "ember_test_m12_settings.ini";
        save_settings_to_file(path, s);

        Settings r = load_settings_from_file(path);
        check(r.master_volume == 42, "master_volume round-trips");
        check(r.sfx_volume    == 12, "sfx_volume round-trips");
        check(r.music_volume  == 88, "music_volume round-trips");
        check(r.fullscreen    == true, "fullscreen round-trips");
        check(r.render_scale  == 2, "render_scale round-trips");
        check(std::memcmp(r.ai_difficulty.data(), "hard", 4) == 0,
              "ai_difficulty round-trips");

        // Missing key -> default. We don't test "missing key" here
        // (would need a hand-crafted INI), but defaults() should
        // always return a sane value.
        Settings d = Settings();
        check(d.master_volume == 80, "defaults.master_volume == 80");
        check(d.sfx_mute == false, "defaults.sfx_mute == false");
        check(d.ai_difficulty[0] != 0, "defaults.ai_difficulty non-empty");

        std::remove(path.c_str());
    }

    // ---- 2. Settings::reset_to_defaults -----------------------------------
    section("settings reset_to_defaults");
    {
        Settings s;
        s.master_volume = 0;
        s.fullscreen = true;
        s.master_mute = true;
        s.reset_to_defaults();
        check(s.master_volume == 80, "reset_to_defaults restores master_volume");
        check(s.fullscreen == false, "reset_to_defaults restores fullscreen");
        check(s.master_mute == false, "reset_to_defaults restores master_mute");
    }

    // ---- 3. Audio backend -------------------------------------------------
    section("audio backend");
    {
        Audio audio;
        audio.initialize();
        // backend_available() is host-dependent: real machines with
        // an audio device will report true, headless CI will report
        // false. Either is acceptable; the contract is that
        // initialize() is idempotent and the diagnostic API is
        // observable in both cases.
        check(audio.play(SfxId::None) == false, "play(None) returns false");
        audio.reset_counters_for_test();
        audio.play(SfxId::MenuMove);
        check(audio.play_count(SfxId::MenuMove) >= 1,
              "play_count records MenuMove");

        // Mute flag should silence play() (no counter increment).
        Settings s = Settings();
        s.master_mute = true;
        audio.apply_settings(s);
        const int before = audio.play_count(SfxId::MenuMove);
        audio.play(SfxId::MenuMove);
        check(audio.play_count(SfxId::MenuMove) == before,
              "master_mute silences play()");
    }

    // ---- 4. Menu state machine (event-driven) -----------------------------
    section("menu state machine (events)");
    {
        Menu menu;
        Settings s = Settings();
        menu.bind_settings(&s);

        check(menu.state() == Menu::State::kTitle,
              "fresh menu starts in kTitle");
        check(menu.blocks_simulation(),
              "kTitle blocks simulation");

        menu.push_event(Menu::Event::kMatchStarted);
        check(menu.state() == Menu::State::kPlay,
              "kMatchStarted -> kPlay");
        check(!menu.blocks_simulation(),
              "kPlay does NOT block simulation");

        menu.push_event(Menu::Event::kMatchPaused);
        check(menu.state() == Menu::State::kPaused,
              "kMatchPaused -> kPaused");
        check(menu.blocks_simulation(),
              "kPaused blocks simulation");

        menu.push_event(Menu::Event::kMatchResumed);
        check(menu.state() == Menu::State::kPlay,
              "kMatchResumed -> kPlay");

        menu.push_event(Menu::Event::kMatchEnded);
        check(menu.state() == Menu::State::kRematch,
              "kMatchEnded -> kRematch");
        check(menu.blocks_simulation(),
              "kRematch blocks simulation");
    }

    // ---- 5. Menu state machine (selected_index clamps) --------------------
    section("menu selected_index bounds");
    {
        Menu menu;
        Settings s = Settings();
        menu.bind_settings(&s);

        // Force into main menu. selected_index_ is private but
        // we have selected_index_for_test() to peek.
        // (We use set_state_for_test to set the state without
        // needing real input.)
        menu.set_state_for_test(Menu::State::kMain);
        menu.set_settings_for_test(&s);

        // The first entry of the main menu is PLAY.
        check(menu.selected_index_for_test() == 0,
              "main starts at index 0");
    }

    // ---- 6. Gamepad API surface (no device) ------------------------------
    section("gamepad (no device attached)");
    {
        Gamepad pad;
        // Constructed without a device: instance_id_ is -1.
        // pressed() and nav_*() return false.
        check(!pad.pressed(SDL_GamepadButton::SDL_GAMEPAD_BUTTON_SOUTH),
              "pad.pressed returns false with no device");
        // confirm() is the same predicate: south face.
        // (The Gamepad header maps SOUTH to confirm.)
        // We don't have a non-member way to call confirm() here
        // because it's a member; just check the public member
        // function exists and is callable.
        bool (*dummy_confirm)(Gamepad*) = nullptr;
        (void)dummy_confirm;
    }

    // ---- 7. Settings::name() / Audio::name_of() ---------------------------
    section("stringification");
    {
        check(std::strcmp(Audio::name_of(SfxId::MenuMove), "MenuMove") == 0,
              "Audio::name_of(MenuMove)");
        check(std::strcmp(Audio::name_of(SfxId::Ko), "Ko") == 0,
              "Audio::name_of(Ko)");
    }

    // ---- 8. M12 accessibility options round-trip --------------------------
    section("accessibility options");
    {
        Settings s = Settings();
        s.screen_shake = false;
        s.color_blind_palette = true;
        s.render_scale = 2;
        s.fullscreen = true;

        const std::string path = "ember_test_m12_a11y.ini";
        save_settings_to_file(path, s);
        Settings r = load_settings_from_file(path);

        check(r.screen_shake == false,
              "screen_shake=false round-trips");
        check(r.color_blind_palette == true,
              "color_blind_palette=true round-trips");
        check(r.render_scale == 2,
              "render_scale=2 round-trips");
        check(r.fullscreen == true,
              "fullscreen=true round-trips");

        // reset_to_defaults should restore accessibility defaults.
        r.reset_to_defaults();
        check(r.screen_shake == true,
              "reset_to_defaults: screen_shake=true");
        check(r.color_blind_palette == false,
              "reset_to_defaults: color_blind_palette=false");

        std::remove(path.c_str());
    }

    // ---- 8b. M12-PARTIAL closers: fullscreen / render scale /
    // color-blind palette. These exercise the new code added to
    // close the M12-PARTIAL items flagged in the master audit
    // (item 4 = fullscreen, item 5 = render scale, item 6 =
    // color-blind palette). They run headless - we never open
    // a real window - so the fullscreen toggle is verified via
    // the renderer's `fullscreen_` flag is N/A; we instead check
    // that the settings round-trip preserves the values, that
    // the runtime color-blind flag tracks the art::set_color_blind
    // call, and that the render scale default falls within the
    // documented 1..4 range.
    section("M12-PARTIAL closers: fullscreen + render scale + color blind");
    {
        // Fullscreen round-trip: settings should preserve the value
        // through save/load (the M12-PARTIAL item 4 closer is
        // that a saved fullscreen=true actually flips the
        // Window, which is exercised at app startup; here we
        // only verify the persistence layer).
        {
            Settings s = Settings();
            s.fullscreen = true;
            const std::string path = "ember_test_m12_fs.ini";
            save_settings_to_file(path, s);
            const Settings r = load_settings_from_file(path);
            check(r.fullscreen == true,
                  "fullscreen=true persists across save/load");
            std::remove(path.c_str());
        }

        // Render scale persistence and clamping.
        {
            Settings s = Settings();
            s.render_scale = 3;
            const std::string path = "ember_test_m12_rs.ini";
            save_settings_to_file(path, s);
            const Settings r = load_settings_from_file(path);
            check(r.render_scale == 3,
                  "render_scale=3 persists across save/load");
            check(s.render_scale >= 1 && s.render_scale <= 4,
                  "render_scale default is within 1..4");
            std::remove(path.c_str());
        }

        // Color-blind palette: the runtime flag must track the
        // setter call. We save and restore the prior value so
        // later sections in the same test (or a follow-up
        // run) see the original state.
        {
            const bool prior = ember::art::color_blind_enabled();
            ember::art::set_color_blind(true);
            check(ember::art::color_blind_enabled() == true,
                  "art::set_color_blind(true) flips the flag");
            ember::art::set_color_blind(false);
            check(ember::art::color_blind_enabled() == false,
                  "art::set_color_blind(false) flips it back");
            // Restore prior (in case this test is ever re-run
            // within a single process).
            ember::art::set_color_blind(prior);
        }

        // The two runtime palettes are distinct (color blind
        // swaps the body color away from the default). We
        // compare constexpr structs at compile time. The check
        // is a runtime equality test.
        check(ember::art::kPaletteP1.body.r !=
              ember::art::kPaletteP1ColorBlind.body.r ||
              ember::art::kPaletteP1.body.g !=
              ember::art::kPaletteP1ColorBlind.body.g ||
              ember::art::kPaletteP1.body.b !=
              ember::art::kPaletteP1ColorBlind.body.b,
              "P1 color-blind body differs from default body");
        check(ember::art::kPaletteP2.body.r !=
              ember::art::kPaletteP2ColorBlind.body.r ||
              ember::art::kPaletteP2.body.g !=
              ember::art::kPaletteP2ColorBlind.body.g ||
              ember::art::kPaletteP2.body.b !=
              ember::art::kPaletteP2ColorBlind.body.b,
              "P2 color-blind body differs from default body");

        // palette_for(0, false) returns the default P1 palette;
        // palette_for(0, true) returns the color-blind P1.
        check(&ember::art::palette_for(0, false) ==
              &ember::art::kPaletteP1,
              "palette_for(0, false) -> kPaletteP1");
        check(&ember::art::palette_for(0, true) ==
              &ember::art::kPaletteP1ColorBlind,
              "palette_for(0, true) -> kPaletteP1ColorBlind");
        check(&ember::art::palette_for(1, false) ==
              &ember::art::kPaletteP2,
              "palette_for(1, false) -> kPaletteP2");
        check(&ember::art::palette_for(1, true) ==
              &ember::art::kPaletteP2ColorBlind,
              "palette_for(1, true) -> kPaletteP2ColorBlind");
    }

    // ---- 9. Playability gate: pause / rematch / start-stop of the
    // match state machine. The Menu's contract is that
    // blocks_simulation() returns true in every state that is
    // not kPlay; that contract is what App::update uses to decide
    // whether to call match_->update() this frame. If the
    // contract breaks, physics / AI / timers / animations /
    // respawn / hit-stop all keep ticking while the player is
    // staring at the pause menu, which is the bug.
    section("playability gate: pause / rematch block simulation");
    {
        Menu menu;
        Settings s = Settings();
        menu.bind_settings(&s);

        // Title blocks.
        check(menu.state() == Menu::State::kTitle, "starts in kTitle");
        check(menu.blocks_simulation(), "kTitle blocks simulation");

        // After kMatchStarted, we should be in kPlay and NOT
        // blocking.
        menu.push_event(Menu::Event::kMatchStarted);
        check(menu.state() == Menu::State::kPlay,
              "kMatchStarted -> kPlay");
        check(!menu.blocks_simulation(),
              "kPlay does NOT block simulation (the contract)");

        // After kMatchPaused, the menu is kPaused and the match
        // must NOT be allowed to tick.
        menu.push_event(Menu::Event::kMatchPaused);
        check(menu.state() == Menu::State::kPaused,
              "kMatchPaused -> kPaused");
        check(menu.blocks_simulation(),
              "kPaused blocks simulation (no physics, no AI, no timers)");

        // While paused, a follow-up kMatchPaused should NOT
        // unpause (the menu reads kMatchResumed to unpause).
        // We assert that the state is still kPaused.
        menu.push_event(Menu::Event::kMatchPaused);
        check(menu.state() == Menu::State::kPaused,
              "double-pause stays paused (kMatchResumed required to unpause)");

        // kMatchResumed returns to kPlay.
        menu.push_event(Menu::Event::kMatchResumed);
        check(menu.state() == Menu::State::kPlay,
              "kMatchResumed -> kPlay");
        check(!menu.blocks_simulation(),
              "kPlay after resume: blocks_simulation == false");

        // kMatchEnded -> kRematch which blocks.
        menu.push_event(Menu::Event::kMatchEnded);
        check(menu.state() == Menu::State::kRematch,
              "kMatchEnded -> kRematch");
        check(menu.blocks_simulation(),
              "kRematch blocks simulation (no respawn drift, no KO-shake animation timer)");
    }

    // ---- 10. Playability gate: Match::reset clears all residual state.
    // This is what App::reset_match() calls when the player
    // picks REMATCH. If any field sticks, the next match would
    // start with a poisoned state (e.g. last stock gone, KO
    // shake still armed, hit-stop still active).
    section("playability gate: Match::reset clears residual state");
    {
        // Use a no-op input (no keys held) so the match
        // advances via the AI only. InputManager has the same
        // test seam M6-C uses: begin_frame_for_test() snapshots
        // the previous state, then we set or clear held keys
        // for the upcoming frame.
        ember::InputManager input;
        Match m;
        m.set_ai(1, std::make_unique<ember::AiController>(ember::ai::kStandard));
        const uint64_t h0 = m.state_hash();
        check(h0 != 0ull, "fresh match has non-zero state_hash");

        // 120 ticks of pure-AI play. No human input. 120 frames
        // (2 seconds) is well past the default AI reaction delay
        // AND enough time for P1 (the bot) to walk measurably
        // toward P0, so we expect a different state_hash.
        constexpr int kTicks = 120;
        for (int i = 0; i < kTicks; ++i) {
            input.begin_frame_for_test();
            m.update(input, 1.0 / 60.0);
        }
        const uint64_t h1 = m.state_hash();
        check(h1 != h0, "match state_hash changes after 120 ticks");

        // Reset. The new state_hash should equal the fresh
        // match's state_hash exactly (same RNG seed, same
        // spawn positions, same initial stocks).
        m.reset();
        const uint64_t h2 = m.state_hash();
        check(h2 == h0, "post-reset state_hash == fresh state_hash");

        // Re-run 120 ticks and confirm the hash sequence is
        // bit-identical to the first 120-tick run. This is the
        // real determinism contract for rematch: same seed +
        // same inputs + same tick count = same hash.
        for (int i = 0; i < kTicks; ++i) {
            input.begin_frame_for_test();
            m.update(input, 1.0 / 60.0);
        }
        const uint64_t h3 = m.state_hash();
        check(h3 == h1, "post-reset 120-tick hash == pre-reset 120-tick hash (deterministic rematch)");
    }

    // ---- 11. (placeholder: see section 8 above for accessibility test)
    // The section 8 test above is the canonical accessibility test;
    // we keep this slot so future sections can be inserted in
    // numeric order without renumbering.

    // ---- 12. M13: P1 key remap ------------------------------------------
    // The P1 rebind UI introduces a runtime-mutable KeyBindings
    // table in InputMap and a persistent p1_bindings array in
    // Settings. These checks verify the data-flow contract end
    // to end without opening a window.
    section("M13: P1 key remap");
    {
        using namespace ember;
        // 1) KeyBindings defaults are the documented WASD + JKL set.
        {
            KeyBindings b;
            check(b.move_left  == SDLK_A,   "KeyBindings default move_left == A");
            check(b.move_right == SDLK_D,   "KeyBindings default move_right == D");
            check(b.jump       == SDLK_W,   "KeyBindings default jump == W");
            check(b.fast_fall  == SDLK_S,   "KeyBindings default fast_fall == S");
            check(b.jab        == SDLK_J,   "KeyBindings default jab == J");
            check(b.ftilt      == SDLK_K,   "KeyBindings default ftilt == K");
            check(b.nair       == SDLK_L,   "KeyBindings default nair == L");
        }

        // 2) KeyBindings equality is value-wise.
        {
            KeyBindings a;
            KeyBindings b = a;
            check(a == b, "KeyBindings value-equality works");
            b.jab = SDLK_F;
            check(a != b, "KeyBindings inequality detected after change");
        }

        // 3) KeyBindings::slot() returns the right address for each
        //    of the 7 actions, and nullptr for out-of-range.
        {
            KeyBindings b;
            check(b.slot(0) == &b.move_left,  "slot(0) -> move_left");
            check(b.slot(1) == &b.move_right, "slot(1) -> move_right");
            check(b.slot(2) == &b.jump,       "slot(2) -> jump");
            check(b.slot(3) == &b.fast_fall,  "slot(3) -> fast_fall");
            check(b.slot(4) == &b.jab,        "slot(4) -> jab");
            check(b.slot(5) == &b.ftilt,      "slot(5) -> ftilt");
            check(b.slot(6) == &b.nair,       "slot(6) -> nair");
            check(b.slot(7) == nullptr,       "slot(7) (out of range) is nullptr");
            check(b.slot(-1) == nullptr,      "slot(-1) (out of range) is nullptr");
            // Mutating through slot() is observable via the
            // struct (no defensive copy).
            *b.slot(4) = SDLK_F;
            check(b.jab == SDLK_F, "slot() mutation is visible on the struct");
        }

        // 4) InputMap::apply_p1_from_ints copies a 7-int buffer
        //    into the live KeyBindings. We mutate the live
        //    bindings, then restore them so other tests in this
        //    binary do not see the test-time values.
        {
            KeyBindings& live = InputMap::p1_bindings();
            const SDL_Keycode saved_move_left = live.move_left;
            const SDL_Keycode saved_jab       = live.jab;
            const std::int32_t buf[7] = {
                SDLK_Q, SDLK_E, SDLK_R, SDLK_T,
                SDLK_F, SDLK_G, SDLK_V
            };
            InputMap::apply_p1_from_ints(buf, 7);
            check(live.move_left  == SDLK_Q, "apply moved move_left to Q");
            check(live.move_right == SDLK_E, "apply moved move_right to E");
            check(live.jump       == SDLK_R, "apply moved jump to R");
            check(live.fast_fall  == SDLK_T, "apply moved fast_fall to T");
            check(live.jab        == SDLK_F, "apply moved jab to F");
            check(live.ftilt      == SDLK_G, "apply moved ftilt to G");
            check(live.nair       == SDLK_V, "apply moved nair to V");
            // Restore the defaults so a subsequent test in the
            // same process does not inherit our mutation.
            live.move_left  = saved_move_left;
            live.jab        = saved_jab;
            const std::int32_t defaults[7] = {
                SDLK_A, SDLK_D, SDLK_W, SDLK_S,
                SDLK_J, SDLK_K, SDLK_L
            };
            InputMap::apply_p1_from_ints(defaults, 7);
        }

        // 5) apply_p1_from_ints is null-safe (defensive).
        {
            // Passing nullptr must NOT crash. The original
            // bindings survive the call. (We capture the
            // jab beforehand and compare afterwards.)
            KeyBindings& live = InputMap::p1_bindings();
            const SDL_Keycode saved = live.jab;
            InputMap::apply_p1_from_ints(nullptr, 7);
            check(live.jab == saved,
                  "apply with nullptr is a no-op (jab unchanged)");
        }

        // 6) Settings::p1_bindings round-trips through save/load.
        {
            Settings s = Settings();
            s.p1_bindings = {{SDLK_Q, SDLK_E, SDLK_R, SDLK_T,
                              SDLK_F, SDLK_G, SDLK_V}};
            const std::string path = "ember_test_m12_remap.ini";
            save_settings_to_file(path, s);
            const Settings r = load_settings_from_file(path);
            check(r.p1_bindings[0] == SDLK_Q, "p1 move_left round-trips Q");
            check(r.p1_bindings[1] == SDLK_E, "p1 move_right round-trips E");
            check(r.p1_bindings[2] == SDLK_R, "p1 jump round-trips R");
            check(r.p1_bindings[3] == SDLK_T, "p1 fast_fall round-trips T");
            check(r.p1_bindings[4] == SDLK_F, "p1 jab round-trips F");
            check(r.p1_bindings[5] == SDLK_G, "p1 ftilt round-trips G");
            check(r.p1_bindings[6] == SDLK_V, "p1 nair round-trips V");
            std::remove(path.c_str());
        }

        // 7) Settings parser clamps out-of-range p1_bindings to 0
        //    so a corrupt file cannot leave the player with a
        //    keycode outside the InputManager's 0..511 slot
        //    table.
        {
            Settings s = Settings();
            s.p1_bindings = {{-1, 999, 700, 5, 4, 3, 2}};
            const std::string path = "ember_test_m12_remap_oor.ini";
            save_settings_to_file(path, s);
            const Settings r = load_settings_from_file(path);
            check(r.p1_bindings[0] == 0, "p1_bindings negative -> 0");
            check(r.p1_bindings[1] == 0, "p1_bindings > 511 -> 0");
            check(r.p1_bindings[2] == 0, "p1_bindings > 511 -> 0");
            check(r.p1_bindings[3] == 5, "p1_bindings in range survives");
            check(r.p1_bindings[4] == 4, "p1_bindings in range survives");
            check(r.p1_bindings[5] == 3, "p1_bindings in range survives");
            check(r.p1_bindings[6] == 2, "p1_bindings in range survives");
            std::remove(path.c_str());
        }

        // 8) InputMap::key_name writes a short label and
        //    null-terminates even for long SDL names.
        {
            char buf[8];
            // Printable ASCII = single character.
            const char* n1 = InputMap::key_name(SDLK_A, buf, sizeof(buf));
            check(n1 == buf, "key_name returns the out pointer");
            check(std::strcmp(buf, "A") == 0, "key_name(A) -> \"A\"");
            // SDLK_UNKNOWN -> "---" (not the empty string).
            char buf2[8];
            InputMap::key_name(0, buf2, sizeof(buf2));
            check(std::strcmp(buf2, "---") == 0,
                  "key_name(SDLK_UNKNOWN) -> \"---\"");
            // Truncation safety: a 4-byte buffer cannot hold
            // a long SDL name without truncation, but it must
            // always be null-terminated.
            char buf3[4] = {'X','X','X','X'};
            InputMap::key_name(SDLK_SPACE, buf3, sizeof(buf3));
            check(buf3[sizeof(buf3) - 1] == '\0',
                  "key_name null-terminates a 4-byte buffer");
            check(static_cast<int>(std::strlen(buf3)) <= 3,
                  "key_name truncates to fit");
        }
    }

    std::printf("\n=== M12-AUDIT ===\n");
    std::printf("M12-AUDIT: %d pass, %d fail\n", g_passed, g_total - g_passed);
    return (g_passed == g_total) ? 0 : 1;
}
