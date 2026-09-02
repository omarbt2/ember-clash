#include "Core/App.h"

#include "Core/Config.h"
#include "Core/Logger.h"
#include "Core/Time.h"
#include "Graphics/Renderer.h"
#include "Graphics/Window.h"
#include "Input/InputManager.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>

#include <cmath>
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

    clock_  = std::make_unique<Clock>();
    input_  = std::make_unique<InputManager>();

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

    // Convenience bindings used by the placeholder scene:
    //   ESC          -> quit
    //   F11          -> toggle fullscreen (placeholder behavior, logs only)
    if (input_->pressed(SDLK_ESCAPE)) {
        running_ = false;
    }
    if (input_->pressed(SDLK_F11)) {
        static_cast<void>(0); // hook reserved for a future toggle
    }
}

void App::update(double dt) {
    // No game state yet. The simulation layer (physics, combat, AI) lands
    // in milestone 2. For now we just keep the loop alive and let the
    // renderer animate a placeholder scene.
    static_cast<void>(dt);
}

void App::render() {
    const double elapsed = clock_->elapsed_seconds();

    renderer_->clear(config::kClearColor);
    draw_placeholder_scene(elapsed);
    renderer_->present();
}

void App::draw_placeholder_scene(double elapsed) {
    // Two fighters (placeholder boxes) bouncing gently so we can visually
    // confirm that the loop, delta time, and rendering are wired up.
    constexpr int kFighterSize = 48;

    const float bob = static_cast<float>(std::sin(elapsed * 2.0) * 12.0);

    // P1 anchored on the left side of the arena floor.
    const float p1_x = 220.0f;
    const float floor_y = static_cast<float>(config::kRenderHeight) - 80.0f - kFighterSize + bob;
    renderer_->draw_filled_rect({p1_x, floor_y,
                                 static_cast<float>(kFighterSize),
                                 static_cast<float>(kFighterSize)},
                                config::kFighterP1);

    // P2 anchored on the right side.
    const float p2_x = static_cast<float>(config::kRenderWidth) - 220.0f - kFighterSize;
    renderer_->draw_filled_rect({p2_x, floor_y,
                                 static_cast<float>(kFighterSize),
                                 static_cast<float>(kFighterSize)},
                                config::kFighterP2);

    // Arena floor.
    const float floor_top = static_cast<float>(config::kRenderHeight) - 80.0f;
    renderer_->draw_filled_rect({0.0f, floor_top,
                                 static_cast<float>(config::kRenderWidth),
                                 80.0f},
                                config::kArenaColor);

    // Accent stripe along the top of the floor so we can see the floor edge.
    renderer_->draw_rect_outline({0.0f, floor_top,
                                  static_cast<float>(config::kRenderWidth),
                                  80.0f},
                                 config::kAccentColor);
}

void App::shutdown() {
    // Order matters only insofar as we want the renderer destroyed before
    // the window. The unique_ptr destructors handle this naturally because
    // they're destroyed in reverse declaration order.
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