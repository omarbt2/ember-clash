#include "Core/App.h"

#include "Core/Config.h"
#include "Core/Logger.h"
#include "Core/Time.h"
#include "Gameplay/Match.h"
#include "Graphics/Renderer.h"
#include "Graphics/Window.h"
#include "Input/InputManager.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>

#include <string>

namespace ember {

App::App() = default;

App::~App() {
    shutdown();
}

bool App::initialize() {
    Logger::info("Ember Clash starting...");

    // Quiet SDL's own logs; we route through our Logger instead.
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_WARN);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        Logger::error(std::string{"SDL_Init failed: "} + SDL_GetError());
        return false;
    }
    sdl_initialized_ = true;

    window_ = std::make_unique<Window>(config::kWindowTitle,
                                       config::kWindowWidth,
                                       config::kWindowHeight);
    if (!window_->is_valid()) {
        return false;
    }

    renderer_ = std::make_unique<Renderer>(*window_);
    if (!renderer_->native()) {
        return false;
    }

    clock_ = std::make_unique<Clock>();
    input_ = std::make_unique<InputManager>();
    match_ = std::make_unique<Match>();

    Logger::info("Initialization complete.");
    return true;
}

int App::run() {
    if (!initialize()) {
        shutdown();
        return 1;
    }

    running_ = true;
    int exit_code = 0;

    while (running_) {
        clock_->update();
        const double dt = clock_->delta_seconds();

        handle_events();

        if (input_->quit_requested()) {
            running_ = false;
        }

        update(dt);
        render();
    }

    shutdown();
    return exit_code;
}

void App::handle_events() {
    input_->update();

    if (input_->pressed(SDLK_ESCAPE)) {
        running_ = false;
    }
}

void App::update(double dt) {
    // Match owns the simulation; App just hands it the input + dt.
    match_->update(*input_, dt);
}

void App::render() {
    renderer_->clear(config::kClearColor);
    match_->render(*renderer_);
    renderer_->present();
}

void App::shutdown() {
    // Order matters only insofar as we want the renderer destroyed before
    // the window. The unique_ptr destructors handle this naturally because
    // they're destroyed in reverse declaration order.
    match_.reset();
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