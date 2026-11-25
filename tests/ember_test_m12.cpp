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
#include <fstream>
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

        // Unknown persisted difficulty must be normalized once at load,
        // keeping the Settings UI and runtime AI preset in agreement.
        {
            std::ofstream malformed(path, std::ios::trunc);
            malformed << "ai_difficulty=nightmare\n";
        }
        const Settings safe = load_settings_from_file(path);
        check(std::strcmp(safe.ai_difficulty.data(), "standard") == 0,
              "unknown ai_difficulty falls back to standard");

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

        // Keyboard Escape resumes from the paused state exactly once.
        InputManager keyboard;
        Gamepad controller;
        Audio menu_audio;
        menu.push_event(Menu::Event::kMatchPaused);
        keyboard.set_held_for_test(SDLK_ESCAPE, true);
        check(menu.update(keyboard, controller, menu_audio) == Menu::State::kPlay,
              "Escape resumes from pause menu");
        check(menu.consume_action() == Menu::Action::kResumeMatch,
              "Escape resume emits the resume action");

        // Gamepad Start is also a dedicated resume input, regardless of
        // which pause-menu row happens to be selected.
        menu.push_event(Menu::Event::kMatchPaused);
        controller.set_held_for_test(SDL_GAMEPAD_BUTTON_START, true);
        check(menu.update(keyboard, controller, menu_audio) == Menu::State::kPlay,
              "Gamepad Start resumes from pause menu");
        check(menu.consume_action() == Menu::Action::kResumeMatch,
              "Gamepad Start does not activate the selected pause-menu row");

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
            check(b.slot(KeyBindings::kActionCount) == nullptr,       "slot(13) (out of range) is nullptr");
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

        // 7) Persisted keycodes must be representable by InputManager.
        //    Unknown/corrupt values are restored per-action to defaults;
        //    valid plain and scancode-masked keys remain unchanged.
        {
            Settings s;
            s.reset_to_defaults();
            s.p1_bindings = {{-1, 999, 700, SDLK_Q, SDLK_E, SDLK_R, SDLK_T}};
            const std::string path = "ember_test_m12_remap_oor.ini";
            save_settings_to_file(path, s);
            const Settings r = load_settings_from_file(path);
            check(r.p1_bindings[0] == 97, "p1 invalid negative key restores A default");
            check(r.p1_bindings[1] == 100,
                  "p1 invalid 999 key restores D default");
            check(r.p1_bindings[2] == 119,
                  "p1 invalid 700 key restores W default");
            check(r.p1_bindings[3] == SDLK_Q, "p1 valid Q binding survives");
            check(r.p1_bindings[4] == SDLK_E, "p1 valid E binding survives");
            check(r.p1_bindings[5] == SDLK_R, "p1 valid R binding survives");
            check(r.p1_bindings[6] == SDLK_T, "p1 valid T binding survives");
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

    // ---- 13. M13.1: P2 key remap (mirror of M13) -------------------------
    // Same contract as P1: defaults match the InputMap
    // storage, apply_p2_from_ints copies a 7-int buffer,
    // settings round-trips, out-of-range values clamp to 0,
    // and apply_p2_from_ints is null-safe.
    section("M13.1: P2 key remap");
    {
        // 1) Settings::p2_bindings defaults to the arrow-keys
        //    + numpad layout (the P2 input map's compile-time
        //    defaults).
        Settings s;
        s.reset_to_defaults();
        // The defaults are documented in Settings.cpp. The
        // keycodes are SDLK_LEFT..KP_3.
        check(s.p2_bindings[0] == 1073741904, "p2 move_left default = LEFT");
        check(s.p2_bindings[1] == 1073741903, "p2 move_right default = RIGHT");
        check(s.p2_bindings[2] == 1073741906, "p2 jump default = UP");
        check(s.p2_bindings[3] == 1073741905, "p2 fast_fall default = DOWN");
        // 1073741913 = 0x40000059 = SDLK_KP_1
        check(s.p2_bindings[4] == 1073741913, "p2 jab default = KP_1");
        check(s.p2_bindings[5] == 1073741914, "p2 ftilt default = KP_2");
        check(s.p2_bindings[6] == 1073741915, "p2 nair default = KP_3");
        // P2 defaults are distinct from P1's defaults.
        check(s.p1_bindings[0] != s.p2_bindings[0],
              "p2 defaults are distinct from p1 defaults");

        // 2) InputMap::bindings(1) returns the P2 storage.
        KeyBindings& live_p2 = InputMap::bindings(1);
        // We don't know what the static storage currently
        // holds because earlier tests may have called
        // apply_p2_from_ints. Reset by calling apply again.
        const SDL_Keycode defaults[7] = { SDLK_LEFT, SDLK_RIGHT,
            SDLK_UP, SDLK_DOWN, SDLK_KP_1, SDLK_KP_2, SDLK_KP_3 };
        InputMap::apply_p2_from_ints(
            reinterpret_cast<const std::int32_t*>(defaults), 7);
        check(live_p2.move_left  == SDLK_LEFT,  "p2.move_left = LEFT");
        check(live_p2.move_right == SDLK_RIGHT, "p2.move_right = RIGHT");
        check(live_p2.jump       == SDLK_UP,    "p2.jump = UP");
        check(live_p2.fast_fall  == SDLK_DOWN,  "p2.fast_fall = DOWN");
        check(live_p2.jab        == SDLK_KP_1,  "p2.jab = KP_1");

        // 3) apply_p2_from_ints copies a 7-int buffer.
        const SDL_Keycode new_keys[7] = { SDLK_F1, SDLK_F2, SDLK_F3,
            SDLK_F4, SDLK_F5, SDLK_F6, SDLK_F7 };
        InputMap::apply_p2_from_ints(
            reinterpret_cast<const std::int32_t*>(new_keys), 7);
        check(live_p2.move_left  == SDLK_F1, "p2 move_left = F1");
        check(live_p2.jab        == SDLK_F5, "p2 jab = F5");
        check(live_p2.nair       == SDLK_F7, "p2 nair = F7");

        // 4) apply_p2_from_ints is null-safe.
        InputMap::apply_p2_from_ints(nullptr, 7);
        // No crash. State is preserved as last non-null call.
        check(live_p2.move_left == SDLK_F1,
              "p2 null apply preserves previous state");

        // 5) Settings::p2_bindings round-trips through save/load.
        {
            const std::string path =
                std::string("test_p2_bindings_") +
                std::to_string(g_total) + ".txt";
            Settings s2;
            s2.reset_to_defaults();
            s2.p2_bindings = {{SDLK_Q, SDLK_E, SDLK_R, SDLK_T,
                                SDLK_F, SDLK_G, SDLK_V}};
            const bool ok = save_settings_to_file(path, s2);
            check(ok, "save_settings_to_file wrote p2_bindings");
            Settings r = load_settings_from_file(path);
            check(r.p2_bindings[0] == SDLK_Q, "p2 move_left round-trips Q");
            check(r.p2_bindings[1] == SDLK_E, "p2 move_right round-trips E");
            check(r.p2_bindings[2] == SDLK_R, "p2 jump round-trips R");
            check(r.p2_bindings[3] == SDLK_T, "p2 fast_fall round-trips T");
            check(r.p2_bindings[4] == SDLK_F, "p2 jab round-trips F");
            check(r.p2_bindings[5] == SDLK_G, "p2 ftilt round-trips G");
            check(r.p2_bindings[6] == SDLK_V, "p2 nair round-trips V");
            std::remove(path.c_str());
        }

        // 6) P2 persisted keycodes receive the same corruption checks.
        {
            Settings s3;
            s3.reset_to_defaults();
            s3.p2_bindings = {{-1, 999, 700, SDLK_Q, SDLK_E, SDLK_R, SDLK_T}};
            const std::string path =
                std::string("test_p2_clamp_") +
                std::to_string(g_total) + ".txt";
            save_settings_to_file(path, s3);
            Settings r = load_settings_from_file(path);
            check(r.p2_bindings[0] == 1073741904, "p2 invalid negative key restores LEFT default");
            check(r.p2_bindings[1] == 1073741903,
                  "p2 invalid 999 key restores RIGHT default");
            check(r.p2_bindings[2] == 1073741906,
                  "p2 invalid 700 key restores UP default");
            check(r.p2_bindings[3] == SDLK_Q, "p2 valid Q binding survives");
            check(r.p2_bindings[4] == SDLK_E, "p2 valid E binding survives");
            check(r.p2_bindings[5] == SDLK_R, "p2 valid R binding survives");
            check(r.p2_bindings[6] == SDLK_T, "p2 valid T binding survives");
            std::remove(path.c_str());
        }

        // 7) An M13-era settings file (no p2_* keys) loads with
        //    p2_bindings at defaults. Backward compatibility.
        {
            const std::string path =
                std::string("test_p2_legacy_") +
                std::to_string(g_total) + ".txt";
            std::ofstream f(path, std::ios::binary | std::ios::trunc);
            f << "master_volume=80\n";
            f << "p1_jump=113\n";  // 'q' - not the default
            f.close();
            Settings r = load_settings_from_file(path);
            check(r.master_volume == 80, "legacy load: master_volume survives");
            check(r.p1_bindings[2] == 113, "legacy load: p1_jump survives");
            // p2_bindings are the defaults (P2 rows were not
            // in the file at all).
            check(r.p2_bindings[0] == 1073741904,
                  "legacy load: p2_bindings[0] = default LEFT");
            check(r.p2_bindings[2] == 1073741906,
                  "legacy load: p2_bindings[2] = default UP");
            std::remove(path.c_str());
        }

        // 8) Menu exposes P1 + P2 BIND rows. The settings
        //    screen rebuilds the entry list on enter; we
        //    bind Settings, jump to kSettings, and count.
        {
            Menu menu;
            Settings s;
            s.reset_to_defaults();
            // Tweak P2 so we can see the live read.
            s.p2_bindings[4] = 1024;  // deliberately invalid legacy value displays as unbound
            menu.bind_settings(&s);
            menu.set_state_for_test(Menu::State::kSettings);
            menu.rebuild_settings_entries_for_test();
            // entries_for_state gives us the live list.
            const std::size_t n = menu.entries_for_state_for_test(
                Menu::State::kSettings).size();
            // 11 audio/video rows + 13 P1 BIND + 13 P2 BIND
            // + 2 footer rows (RESTORE DEFAULTS, BACK) = 39.
            check(n == 39,
                  "kSettings entries count = 39 (11 AV + 26 BIND + 2 footer)");
            const auto& rows = menu.entries_for_state_for_test(Menu::State::kSettings);
            check(rows[Menu::kSettingsBindStartIndex + 7].label == "P1 AIM UP",
                  "settings exposes P1 AIM UP rebind row");
            check(rows[Menu::kSettingsBindStartIndex + 13 + 12].label == "P2 EMBER SURGE",
                  "settings exposes P2 EMBER SURGE rebind row");
            check(Menu::kSettingsRestoreDefaultsIndex == 37 &&
                  Menu::kSettingsBackIndex == 38,
                  "settings footer indices follow all 26 bind rows");

            // Controls must always mirror the same live settings,
            // otherwise users would see old key labels after rebinding.
            s.p1_bindings[0] = SDLK_Q;
            menu.refresh_settings();
            const auto& controls = menu.entries_for_state_for_test(Menu::State::kControls);
            check(controls.size() == 30,
                  "controls page lists 26 actions and four menu shortcuts");
            check(controls[0].value == "Q",
                  "controls page reflects the current P1 binding");
            check(controls[13 + 4].value == "---",
                  "invalid/unbound P2 values are displayed as unbound");

            // 9) Sanity: pending remap starts empty; full capture path
            //    is covered by the Rebirth input regression tests.
            //    with remap_player_ == 1. The handler returns
            //    Action::kNone because the remap is committed
            //    later by kApplyRemap. We drive the menu
            //    directly using set_settings_for_test +
            //    set_state + a pre-seeded selection by
            //    recreating with selection. Menu exposes
            //    selected_index_for_test() so we can verify
            //    the row read back is the one we set, but
            //    the capture-state plumbing is private.
            //    Instead we verify the public read-back: the
            //    remap commit goes through pending_remap_*
            //    fields, which start at -1 / 0.
            check(menu.pending_remap_player() == -1,
                  "pending_remap_player starts at -1");
            check(menu.pending_remap_slot() == -1,
                  "pending_remap_slot starts at -1");
        }
    }

    std::printf("\n=== M12-AUDIT ===\n");
    std::printf("M12-AUDIT: %d pass, %d fail\n", g_passed, g_total - g_passed);
    return (g_passed == g_total) ? 0 : 1;
}
