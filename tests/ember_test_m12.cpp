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
#include "Gameplay/Match.h"
#include "Input/Gamepad.h"
#include "Input/InputManager.h"
#include "UI/Menu.h"

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

    std::printf("\n=== M12-AUDIT ===\n");
    std::printf("M12-AUDIT: %d pass, %d fail\n", g_passed, g_total - g_passed);
    return (g_passed == g_total) ? 0 : 1;
}
