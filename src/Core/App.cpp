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

    // M13: push the saved P1 key bindings into the live
    // InputMap. The InputMap's static storage starts at the
    // compile-time defaults; without this call a user who
    // rebinds A to a non-default key would still see the
    // fighter responding to the old key on the very first
    // frame. The apply function is a no-op if the array is
    // empty (it isn't, but the guard makes the intent clear).
    InputMap::apply_p1_from_ints(
        settings_.p1_bindings.data(),
        settings_.p1_bindings.size());

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
            art::set_color_blind(settings_.color_blind_palette);
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
            // M13: the player committed a key rebind in the
            // remap-capture sub-state. The menu stored the slot
            // index and keycode; we copy them into the live
            // Settings, push the new bindings into InputMap so
            // the next sampled frame uses them, mark the
            // settings file dirty, and immediately flush so
            // a crash before the next menu close does not lose
            // the rebind.
            const int slot = menu_->pending_remap_slot();
            const int kc   = menu_->pending_remap_keycode();
            if (slot >= 0 && slot < 7 && kc >= 0 && kc <= 511) {
                settings_.p1_bindings[static_cast<std::size_t>(slot)] =
                    static_cast<std::int32_t>(kc);
                InputMap::apply_p1_from_ints(
                    settings_.p1_bindings.data(),
                    settings_.p1_bindings.size());
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
