#include "Core/App.h"

#include "Audio/Audio.h"
#include "Core/Config.h"
#include "Core/Logger.h"
#include "Core/Time.h"
#include "Gameplay/AiConfig.h"
#include "Gameplay/AiController.h"
#include "World/Arena.h"
#include "Gameplay/ArtConfig.h"
#include "Gameplay/InputMap.h"
#include "Gameplay/Match.h"
#include "Gameplay/Fighter.h"
#include "Graphics/Renderer.h"
#include "Graphics/Window.h"
#include "Input/Gamepad.h"
#include "Input/Haptics.h"
#include "Input/InputManager.h"
#include "UI/Menu.h"
#include "UI/BitmapFont.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace ember {

App::App() = default;

App::~App() {
    if (settings_dirty_) {
        flush_settings();
    }
    shutdown();
}

bool App::initialize() {
    Logger::info("Ember Clash starting...");

    SDL_SetLogPriorities(SDL_LOG_PRIORITY_WARN);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        Logger::error(std::string{"SDL_Init failed: "} + SDL_GetError());
        return false;
    }
    sdl_initialized_ = true;

    // Load the persistent settings (or defaults if no file). We
    // do this BEFORE creating the window so a saved fullscreen=true
    // makes the game boot directly into fullscreen rather than
    // briefly flashing windowed.
    settings_ = load_settings_from_file(default_settings_path());

    window_ = std::make_unique<Window>(config::kWindowTitle,
                                       config::kWindowWidth,
                                       config::kWindowHeight);
    if (!window_->is_valid()) {
        return false;
    }
    if (settings_.fullscreen) {
        window_->set_fullscreen(true);
    }

    renderer_ = std::make_unique<Renderer>(*window_);
    if (!renderer_->native()) {
        return false;
    }

    // Apply the saved render scale on boot. Settings stores an
    // integer (1..4); the renderer clamps internally. A value of
    // 1 is the 1:1 identity default; higher values cause
    // SDL_RenderSetScale to up-scale the output to the window.
    if (settings_.render_scale < 1) settings_.render_scale = 1;
    if (settings_.render_scale > 4) settings_.render_scale = 4;
    renderer_->set_render_scale(static_cast<float>(settings_.render_scale));

    clock_   = std::make_unique<Clock>();
    input_   = std::make_unique<InputManager>();
    gamepad_ = std::make_unique<Gamepad>();
    // M14: Haptics binds to the gamepad so play_*() can route
    // rumble effects. The Haptics instance is constructed
    // before the Audio object so its destructor runs after
    // the Audio one (App destroys members in reverse
    // construction order); the order matters because
    // Haptics' destructor issues a final stop_rumble() and
    // we want that to run while Gamepad is still alive.
    haptics_ = std::make_unique<Haptics>();
    haptics_->bind_gamepad(gamepad_.get());
    audio_   = std::make_unique<Audio>();
    audio_->initialize();
    menu_    = std::make_unique<Menu>();

    audio_->apply_settings(settings_);
    menu_->bind_settings(&settings_);

    // Apply art-related settings (color-blind palette) BEFORE the
    // first match is constructed, so the very first sprite is
    // already in the correct palette. The flag is consulted at
    // Fighter construction time.
    art::set_color_blind(settings_.color_blind_palette);

    // M13 / M13.1: push the saved P1 and P2 key bindings
    // into the live InputMap. The InputMap's static storage
    // starts at the compile-time defaults; without this call
    // a user who rebinds A to a non-default key would still
    // see the fighter responding to the old key on the very
    // first frame. P2 is consumed by the AI in V1.0 so the
    // remap has no immediate in-game effect, but the AI
    // controller reads the same KeyBindings storage for
    // future 2P local mode and the settings file must round-
    // trip the values regardless.
    InputMap::apply_p1_from_ints(
        settings_.p1_bindings.data(),
        settings_.p1_bindings.size());
    InputMap::apply_p2_from_ints(
        settings_.p2_bindings.data(),
        settings_.p2_bindings.size());

    // Create the first match instance. The menu starts in
    // kTitle and gates whether this match is allowed to tick.
    match_ = std::make_unique<Match>();

    // Inject an AI on P2 so the player has an opponent right
    // away. The difficulty is read from settings (which
    // defaults to "standard" on first launch).
    {
        const std::string diff(settings_.ai_difficulty.data());
        match_->set_ai(1, std::make_unique<AiController>(ai::from_name(diff)));
    }

    Logger::info("Initialization complete.");
    return true;
}

int App::run() {
    if (!initialize()) {
        shutdown();
        return 1;
    }

    running_ = true;

    // P17: fixed-timestep accumulator. PhysicsConfig and
    // CombatConfig constants are all in "per 60 Hz tick" units
    // (e.g. kWalkSpeed=6.0 px/tick, kHitSparkFrames=6 ticks,
    // kIntroBannerReadyFrames=60 ticks = 1 second). Running the
    // simulation more than 60 times per second on a high-refresh
    // display made every action, attack, and AI decision roughly
    // N times faster than the numbers promised. Conversely,
    // running it at 30 fps (windowed, dragging) would make the
    // game feel sluggish. The accumulator below is the standard
    // "Glenn Fiedler" fix: render as fast as the display can
    // show frames, simulate exactly kTargetFps times per second
    // (or in catch-up chunks after a stall, capped at
    // kMaxSimTicksPerFrame so a long pause cannot time-warp the
    // match).
    const double sim_step = clock_->fixed_step_seconds();
    sim_accumulator_      = 0.0;

    while (running_) {
        clock_->update();
        const double frame_dt = clock_->delta_seconds();
        sim_accumulator_     += frame_dt;

        // 1) Handle events ONCE per render frame so the player
        //    can press quit / pause even on a 240 Hz monitor
        //    without queueing four quit events per visible frame.
        handle_events();
        if (input_->quit_requested()) {
            running_ = false;
            break;
        }

        // 2) Run 0..N sim ticks at exactly sim_step each.
        int sim_ticks = 0;
        while (sim_accumulator_ >= sim_step
               && sim_ticks < kMaxSimTicksPerFrame) {
            // update() with a fixed sim_step; the match's
            // internal counters (match_startup_ticks_,
            // live_ticks_, attack cooldowns, AI state_ticks_)
            // are all expressed in ticks so this is correct
            // and deterministic.
            update(sim_step);
            sim_accumulator_ -= sim_step;
            ++sim_ticks;
        }
        // If we hit the cap (long stall, breakpoint, OS hiccup),
        // discard the leftover time so the next frame does not
        // also try to catch up. The player will see a tiny jump
        // which is much better than a 15-tick time-warp.
        if (sim_ticks >= kMaxSimTicksPerFrame) {
            sim_accumulator_ = 0.0;
        }
        // 3) Render EXACTLY once per visible frame, using the
        //    post-tick state. The previous code called step()
        //    which rendered after every sim tick, causing
        //    duplicate frames on a 144/240 Hz monitor and
        //    burn-in of sprite state in SDL's texture cache.
        render();
    }

    shutdown();
    return 0;
}

void App::step(double dt) {
    handle_events();

    if (input_->quit_requested()) {
        running_ = false;
        return;
    }

    update(dt);
    render();
}

void App::request_exit() {
    running_ = false;
}

void App::handle_events() {
    input_->update();
    gamepad_->update(*input_);
}

void App::update(double dt) {
    // Snapshot the match state BEFORE ticking so we can detect
    // edge transitions (a hit landing this frame, the phase
    // flipping to MatchOver). M14 haptics hooks off these
    // edges.
    const Match::Phase prev_phase = match_ ? match_->phase()
                                           : Match::Phase::Fighting;
    const bool prev_hit_valid = match_
        ? match_->last_hit_position_for_test().valid
        : false;

    // Capture this before the menu consumes Escape/Start: when the user
    // resumes from a paused screen, the menu may transition back to kPlay
    // during update(). That same key edge must not immediately pause again.
    const bool menu_was_playing = menu_ && menu_->state() == Menu::State::kPlay;
    if (menu_) {
        menu_->update(*input_, *gamepad_, *audio_);
    }

    // Push a MatchOver event into the menu if the match ended
    // since the last frame. We do this here (not inside Match)
    // so the menu stays a pure consumer of events.
    if (match_ && match_->phase() == Match::Phase::MatchOver
        && menu_ && menu_->state() == Menu::State::kPlay) {
        menu_->push_event(Menu::Event::kMatchEnded);
    }

    // The pause toggle only applies to a frame that STARTED in Play.
    // Escape/Start used to resume from the pause screen is consumed by
    // the menu and must not immediately open the pause screen again.
    if (menu_ && menu_was_playing && menu_->state() == Menu::State::kPlay
        && (input_->menu_pause()
            || gamepad_->pressed(SDL_GAMEPAD_BUTTON_START))) {
        audio_->play(SfxId::Pause);
        menu_->push_event(Menu::Event::kMatchPaused);
    }

    handle_menu_action();

    // Free Training transport controls: F5 starts a fresh capture, F6
    // stops-and-replays the capture (or restarts an existing replay), and
    // F7 clears it. These controls are deliberately active only in the
    // dedicated no-CPU mode so they cannot steal ordinary gameplay keys.
    if (free_training_ && match_ && menu_
        && menu_->state() == Menu::State::kPlay) {
        if (input_->pressed(SDLK_F5)) {
            match_->start_training_recording();
        }
        if (input_->pressed(SDLK_F6)) {
            if (match_->training_is_recording()) match_->stop_training_recording();
            match_->play_training_recording();
        }
        if (input_->pressed(SDLK_F7)) {
            match_->clear_training_recording();
        }
    }

    // Tick the match ONLY when the menu is not blocking the
    // simulation. The pause / rematch / settings screens all
    // stop the match from advancing so the player can take
    // their time deciding.
    if (match_ && menu_ && !menu_->blocks_simulation()) {
        match_->update(*input_, dt);
    }

    // M14 haptics: route game events to the controller. The
    // routing is intentionally simple - we do not try to
    // detect every single attack type; we just fire the
    // "strong hit" cue on every fresh hit, and the "KO" cue
    // on the phase transition. Edge detection (prev vs
    // current) is essential: per-frame detection would
    // either re-trigger every tick (causing a permanent
    // rumble) or fire while the menu is open.
    if (match_ && haptics_) {
        const auto last_hit = match_->last_hit_position_for_test();
        if (last_hit.valid && !prev_hit_valid) {
            haptics_->play_strong_hit();
        }
        if (match_->phase() == Match::Phase::MatchOver
            && prev_phase != Match::Phase::MatchOver) {
            haptics_->play_ko();
        }
    }
    if (haptics_) haptics_->update();

    // P15: route game events to the audio layer the same way
    // we route them to the haptics layer. The edge detection
    // (prev vs current last_hit_) prevents per-frame spam.
    // A "strong hit" SFX fires on every fresh hit; the "KO"
    // cue fires on the match-end transition. The intro banner
    // is silent by design so the player can read "READY...".
    if (match_ && audio_) {
        const auto last_hit = match_->last_hit_position_for_test();
        if (last_hit.valid && !prev_hit_valid) {
            // HitHeavy is the strong cue; the lighter cues
            // (HitLight, HitSpark) live in the bank for
            // future per-attack differentiation.
            audio_->play(SfxId::HitHeavy);
        }
        if (match_->phase() == Match::Phase::MatchOver
            && prev_phase != Match::Phase::MatchOver) {
            audio_->play(SfxId::Ko);
        }
    }

    if (settings_dirty_) {
        last_settings_flush_ += dt;
        if (last_settings_flush_ > 1.0) {
            flush_settings();
            last_settings_flush_ = 0.0;
        }
    }
}

void App::render() {
    // P17: compute the camera position so the action stays centred
    // and the fighters feel close to the camera. The previous
    // "no camera" code showed the entire 1280x720 stage at once,
    // which on a 1920x1080 monitor produced tiny letterboxed
    // fighters (~96x128 px) and on a 4K monitor made them nearly
    // invisible. The camera is render-only: it does not affect
    // physics, hitbox resolution, or test state.
    if (match_) {
        // 1) Find the focus point. Use the midpoint of the two
        //    live fighters; if one is dead, anchor on the live
        //    one; if both are dead, hold the last camera
        //    position so the world does not jump.
        const Fighter& f0 = match_->fighter(0);
        const Fighter& f1 = match_->fighter(1);
        bool f0_alive = !f0.is_dead();
        bool f1_alive = !f1.is_dead();
        double focus_x;
        double focus_y;
        if (f0_alive && f1_alive) {
            focus_x = (f0.body().x + f1.body().x) * 0.5;
            focus_y = (f0.body().y + f1.body().y) * 0.5;
        } else if (f0_alive) {
            focus_x = f0.body().x;
            focus_y = f0.body().y;
        } else if (f1_alive) {
            focus_x = f1.body().x;
            focus_y = f1.body().y;
        } else {
            focus_x = camera_x_ + static_cast<double>(config::kRenderWidth) * 0.5;
            focus_y = camera_y_ + static_cast<double>(config::kRenderHeight) * 0.5;
        }
        // Add a vertical bias so the floor is in the lower third
        // of the screen (Smash convention). A bias of +120 puts
        // the focus point slightly above centre, so the floor
        // shows below and the sky above.
        focus_y += 120.0;

        // 2) Smooth toward the focus. A 1.0 factor would snap;
        //    0.18 feels responsive but not jittery at 60Hz.
        const double kFollowStrength = 0.18;
        // P17: the render scale is settings_.render_scale (1 by
        // default) times the match-zoom 1.3x. The visible viewport
        // in world space is therefore (kRenderWidth / world_scale)
        // by (kRenderHeight / world_scale). Centring a focus point
        // on the screen means offsetting the camera by
        //   (focus - viewport_size / 2)
        // so the rendered position (focus - view) lands in the
        // middle of the visible window after SDL's scale.
        const double world_scale =
            std::max(0.001,
                static_cast<double>(settings_.render_scale) * 1.3);
        const double visible_w =
            static_cast<double>(config::kRenderWidth)  / world_scale;
        const double visible_h =
            static_cast<double>(config::kRenderHeight) / world_scale;
        const double target_cam_x = focus_x - visible_w * 0.5;
        const double target_cam_y = focus_y - visible_h * 0.5;
        camera_x_ += (target_cam_x - camera_x_) * kFollowStrength;
        camera_y_ += (target_cam_y - camera_y_) * kFollowStrength;

        // 3) Clamp so the camera never reveals beyond the arena
        //    edges. The world spans [0, kRenderWidth] x
        //    [0, kRenderHeight]; the camera (top-left) must
        //    stay inside the arena minus the visible viewport
        //    so the world always covers the full screen.
        if (camera_x_ < 0.0) camera_x_ = 0.0;
        if (camera_y_ < 0.0) camera_y_ = 0.0;
        const double max_cam_x =
            std::max(0.0,
                static_cast<double>(config::kRenderWidth) - visible_w);
        const double max_cam_y =
            std::max(0.0,
                static_cast<double>(config::kRenderHeight) - visible_h);
        if (camera_x_ > max_cam_x) camera_x_ = max_cam_x;
        if (camera_y_ > max_cam_y) camera_y_ = max_cam_y;
    }

    // M12 accessibility: when screen_shake is disabled, the post-
    // KO shake amount is clamped to zero. The match's
    // ko_shake_offset_*() functions are the authoritative source;
    // we just choose whether to forward them to the renderer.
    if (match_) {
        float shake_x = 0.0f;
        float shake_y = 0.0f;
        if (settings_.screen_shake) {
            shake_x = match_->ko_shake_offset_x();
            shake_y = match_->ko_shake_offset_y();
        }
        renderer_->set_view_offset(
            static_cast<float>(camera_x_) + shake_x,
            static_cast<float>(camera_y_) + shake_y);
    }
    renderer_->clear(config::kClearColor);

    // P17: when the match is playing, apply a 1.3x render scale so
    // the fighters feel "close to camera" instead of tiny dots in
    // a 1280x720 sea. The scale is applied via the renderer's
    // own set_render_scale (which forwards to SDL_RenderSetScale)
    // and is RESET to 1.0 before the HUD/menu render pass so
    // screen-space UI stays readable. Without this reset, the
    // HUD would also blow up by 1.3x and overlap the fighters.
    const bool apply_match_zoom =
        match_ && (menu_ == nullptr
                   || menu_->state() == Menu::State::kPlay);
    if (apply_match_zoom) {
        renderer_->set_render_scale(
            static_cast<float>(settings_.render_scale) * 1.3f);
    } else {
        renderer_->set_render_scale(
            static_cast<float>(settings_.render_scale));
    }

    // Decide what to draw: in kPlay we just draw the match. In
    // every other state we draw the match behind a translucent
    // menu overlay so the screen does not jump to a flat panel.
    if (menu_ && menu_->state() != Menu::State::kPlay) {
        if (match_) {
            // Match::render draws the arena, fighters, AND the
            // HUD in a single call, so the zoom above applies to
            // all of them. The menu (which is screen-space and
            // would look wrong at 1.3x) is reset to 1.0 below.
            match_->render(*renderer_);
        }
        // P17: reset the render scale BEFORE the menu so the
        // menu UI is drawn at the user's selected render_scale
        // (1.0 by default), not the match-zoom 1.3x.
        renderer_->set_render_scale(
            static_cast<float>(settings_.render_scale));
        menu_->render(*renderer_);
    } else if (match_) {
        match_->render(*renderer_);
        if (free_training_) {
            char status[96]{};
            const char* state = match_->training_is_recording() ? "REC"
                : match_->training_is_playing() ? "PLAYBACK" : "READY";
            std::snprintf(status, sizeof(status),
                "TRAINING %s  %zu TICKS  F5 REC  F6 REPLAY  F7 CLEAR",
                state, match_->training_recorded_ticks());
            const float shake_x = settings_.screen_shake
                ? match_->ko_shake_offset_x() : 0.0f;
            const float shake_y = settings_.screen_shake
                ? match_->ko_shake_offset_y() : 0.0f;
            renderer_->set_view_offset(0.0f, 0.0f);
            renderer_->set_render_scale(static_cast<float>(settings_.render_scale));
            ui::draw_text(*renderer_, 18, 18, status,
                          config::Color{255, 224, 128, 255});
            renderer_->set_view_offset(static_cast<float>(camera_x_) + shake_x,
                                       static_cast<float>(camera_y_) + shake_y);
            renderer_->set_render_scale(static_cast<float>(settings_.render_scale) * 1.3f);
        }
    }

    // P17: always restore the user's render_scale before
    // present() so the next frame's clear() and any future
    // screen-space draws are at the correct scale.
    renderer_->set_render_scale(
        static_cast<float>(settings_.render_scale));

    renderer_->present();
}

void App::handle_menu_action() {
    if (!menu_) return;
    const Menu::Action a = menu_->consume_action();
    if (a == Menu::Action::kNone) return;

    switch (a) {
        case Menu::Action::kStartMatch:
            local_two_player_ = false;
            free_training_ = false;
            start_match();
            break;
        case Menu::Action::kStartLocalTwoPlayer:
            local_two_player_ = true;
            free_training_ = false;
            start_match();
            break;
        case Menu::Action::kStartTraining:
            local_two_player_ = false;
            free_training_ = true;
            start_match();
            break;
        case Menu::Action::kReturnToTitle:
            if (match_) match_->reset();
            menu_->set_state(Menu::State::kTitle);
            break;
        case Menu::Action::kRestartMatch:
            reset_match();
            break;
        case Menu::Action::kResumeMatch:
            resume_match();
            break;
        case Menu::Action::kSaveSettings:
            settings_dirty_ = true;
            flush_settings();
            InputMap::apply_p1_from_ints(
                settings_.p1_bindings.data(), settings_.p1_bindings.size());
            InputMap::apply_p2_from_ints(
                settings_.p2_bindings.data(), settings_.p2_bindings.size());
            if (menu_) menu_->refresh_settings();
            audio_->apply_settings(settings_);
            // Re-apply render scale (in case the user edited it
            // inline via the settings screen, which mutates the
            // int but does not yet push it to the renderer).
            if (renderer_) {
                renderer_->set_render_scale(
                    static_cast<float>(settings_.render_scale));
            }
            if (window_) {
                window_->set_fullscreen(settings_.fullscreen);
            }
            // P15: color-blind toggle must be live. set_color_blind()
            // only updates the runtime flag, but the fighter sprites
            // were built once at construction with the previous
            // palette. We rebuild each fighter's sprite now so the
            // change is visible this frame, not on the next rematch.
            art::set_color_blind(settings_.color_blind_palette);
            if (match_) {
                match_->fighter(0).rebuild_sprite_for_palette();
                match_->fighter(1).rebuild_sprite_for_palette();
            }
            break;
        case Menu::Action::kRestoreDefaultSettings:
            settings_.reset_to_defaults();
            settings_dirty_ = true;
            flush_settings();
            InputMap::apply_p1_from_ints(
                settings_.p1_bindings.data(), settings_.p1_bindings.size());
            InputMap::apply_p2_from_ints(
                settings_.p2_bindings.data(), settings_.p2_bindings.size());
            if (menu_) menu_->refresh_settings();
            audio_->apply_settings(settings_);
            // Re-apply render scale in case the default differs
            // from the live value.
            if (renderer_) {
                renderer_->set_render_scale(
                    static_cast<float>(settings_.render_scale));
            }
            if (window_) {
                window_->set_fullscreen(settings_.fullscreen);
            }
            art::set_color_blind(settings_.color_blind_palette);
            if (match_) {
                match_->fighter(0).rebuild_sprite_for_palette();
                match_->fighter(1).rebuild_sprite_for_palette();
            }
            break;
        case Menu::Action::kToggleFullscreen:
            // Real OS-level fullscreen switch via SDL. The window
            // is recreated-in-place by SDL_SetWindowFullscreen so
            // our Window::set_fullscreen() just toggles the flag
            // and calls into SDL; the renderer does not need to
            // be reinitialised because it draws into the window's
            // logical surface (size-independent).
            settings_.fullscreen = !settings_.fullscreen;
            if (window_) window_->set_fullscreen(settings_.fullscreen);
            settings_dirty_ = true;
            break;
        case Menu::Action::kApplyRenderScale: {
            // Settings stored an integer 1..4. Push it into the
            // renderer so subsequent frames use the new scale.
            const int s = settings_.render_scale;
            const float sf = static_cast<float>(s < 1 ? 1 : s);
            if (renderer_) renderer_->set_render_scale(sf);
            break;
        }
        case Menu::Action::kApplyRemap: {
            // M13 / M13.1: the player committed a key rebind
            // in the remap-capture sub-state. The menu stored
            // the player index, slot index, and keycode; we
            // copy them into the live Settings (picking the
            // correct array by player index), push the new
            // bindings into InputMap so the next sampled
            // frame uses them, mark the settings file dirty,
            // and immediately flush so a crash before the
            // next menu close does not lose the rebind.
            const int player = menu_->pending_remap_player();
            const int slot   = menu_->pending_remap_slot();
            const int kc     = menu_->pending_remap_keycode();
            if (slot >= 0 && slot < KeyBindings::kActionCount
                && InputManager::is_valid_keycode(static_cast<SDL_Keycode>(kc))
                && (player == 0 || player == 1)) {
                if (player == 0) {
                    settings_.p1_bindings[static_cast<std::size_t>(slot)] =
                        static_cast<std::int32_t>(kc);
                    InputMap::apply_p1_from_ints(
                        settings_.p1_bindings.data(),
                        settings_.p1_bindings.size());
                } else {
                    settings_.p2_bindings[static_cast<std::size_t>(slot)] =
                        static_cast<std::int32_t>(kc);
                    InputMap::apply_p2_from_ints(
                        settings_.p2_bindings.data(),
                        settings_.p2_bindings.size());
                }
                settings_dirty_ = true;
                flush_settings();
                if (menu_) menu_->refresh_settings();
            }
            break;
        }
        case Menu::Action::kRequestQuit:
            running_ = false;
            break;
        case Menu::Action::kNone:
            break;
    }
}

void App::start_match() {
    if (match_) {
        const int arena_index = menu_ ? menu_->selected_arena_index() : 0;
        const auto arena_id = static_cast<Arena::Id>(arena_index);
        match_->set_arena(arena_id);
        if (local_two_player_ || free_training_) {
            match_->set_ai(1, nullptr);
        } else {
            const std::string diff(settings_.ai_difficulty.data());
            match_->set_ai(1, std::make_unique<AiController>(ai::from_name(diff)));
        }
    }
    if (menu_)  menu_->push_event(Menu::Event::kMatchStarted);
    audio_->play(SfxId::MatchStart);
}

void App::end_match() {
    if (menu_) menu_->push_event(Menu::Event::kMatchEnded);
}

void App::pause_match() {
    if (menu_) menu_->push_event(Menu::Event::kMatchPaused);
}

void App::resume_match() {
    if (menu_) menu_->push_event(Menu::Event::kMatchResumed);
}

void App::reset_match() {
    if (match_) match_->reset();
    if (menu_)  menu_->push_event(Menu::Event::kMatchStarted);
    audio_->play(SfxId::Rematch);
}

void App::flush_settings() {
    save_settings_to_file(default_settings_path(), settings_);
    settings_dirty_ = false;
}

void App::shutdown() {
    match_.reset();
    menu_.reset();
    audio_.reset();
    gamepad_.reset();
    renderer_.reset();
    window_.reset();
    input_.reset();
    clock_.reset();

    if (sdl_initialized_) {
        SDL_Quit();
        sdl_initialized_ = false;
    }
}

} // namespace ember
