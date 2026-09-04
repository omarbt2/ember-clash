#include "Core/App.h"

#include "Audio/Audio.h"
#include "Core/Config.h"
#include "Core/Logger.h"
#include "Core/Time.h"
#include "Gameplay/AiConfig.h"
#include "Gameplay/AiController.h"
#include "Gameplay/ArtConfig.h"
#include "Gameplay/InputMap.h"
#include "Gameplay/Match.h"
#include "Graphics/Renderer.h"
#include "Graphics/Window.h"
#include "Input/Gamepad.h"
#include "Input/Haptics.h"
#include "Input/InputManager.h"
#include "UI/Menu.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>

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
        ai::Difficulty bot{};
        if      (diff == "easy") bot = ai::kEasy;
        else if (diff == "hard") bot = ai::kHard;
        else                     bot = ai::kStandard;
        match_->set_ai(1, std::make_unique<AiController>(bot));
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

    while (running_) {
        clock_->update();
        step(clock_->delta_seconds());
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
    gamepad_->update();
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

    // The pause toggle. P (keyboard) or gamepad Start pressed
    // while in Play -> push the paused event. While paused,
    // the same input resumes (the menu's navigate() handles
    // the resume confirmation through the pause menu).
    if (menu_ && menu_->state() == Menu::State::kPlay
        && (input_->menu_pause()
            || gamepad_->pressed(SDL_GAMEPAD_BUTTON_START))) {
        audio_->play(SfxId::Pause);
        menu_->push_event(Menu::Event::kMatchPaused);
    }

    handle_menu_action();

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
    // M12 accessibility: when screen_shake is disabled, the post-
    // KO shake amount is clamped to zero. The match's
    // ko_shake_offset_*() functions are the authoritative source;
    // we just choose whether to forward them to the renderer.
    if (match_) {
        if (settings_.screen_shake) {
            renderer_->set_view_offset(
                match_->ko_shake_offset_x(),
                match_->ko_shake_offset_y());
        } else {
            renderer_->set_view_offset(0, 0);
        }
    }
    renderer_->clear(config::kClearColor);

    // Decide what to draw: in kPlay we just draw the match. In
    // every other state we draw the match behind a translucent
    // menu overlay so the screen does not jump to a flat panel.
    if (menu_ && menu_->state() != Menu::State::kPlay) {
        if (match_) {
            match_->render(*renderer_);
        }
        menu_->render(*renderer_);
    } else if (match_) {
        match_->render(*renderer_);
    }

    renderer_->present();
}

void App::handle_menu_action() {
    if (!menu_) return;
    const Menu::Action a = menu_->consume_action();
    if (a == Menu::Action::kNone) return;

    switch (a) {
        case Menu::Action::kStartMatch:
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
            if (slot >= 0 && slot < 7 && kc >= 0 && kc <= 511
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
    if (match_) match_->reset();
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
