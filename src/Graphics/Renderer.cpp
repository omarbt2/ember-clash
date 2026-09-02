#include "Graphics/Renderer.h"

#include "Core/Logger.h"
#include "Graphics/Window.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include <string>

namespace ember {

namespace {

void set_draw_color(SDL_Renderer* r, config::Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

} // namespace

Renderer::Renderer(Window& window) {
    SDL_Window* raw_window = window.native();
    if (!raw_window) {
        Logger::error("Renderer: window is invalid.");
        return;
    }

    SDL_Renderer* raw = SDL_CreateRenderer(raw_window, nullptr);
    if (!raw) {
        Logger::error(std::string{"SDL_CreateRenderer failed: "} + SDL_GetError());
        return;
    }

    renderer_ = raw;

    // We render at the logical resolution; SDL scales it to the actual
    // window size for us.
    if (!SDL_SetRenderLogicalPresentation(raw,
                                          config::kRenderWidth,
                                          config::kRenderHeight,
                                          SDL_LOGICAL_PRESENTATION_LETTERBOX)) {
        Logger::warn(std::string{"SDL_SetRenderLogicalPresentation: "} + SDL_GetError());
    }

    SDL_GetCurrentRenderOutputSize(raw, &out_w_, &out_h_);
}

Renderer::~Renderer() {
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
}

void Renderer::clear(config::Color color) {
    if (!renderer_) return;
    set_draw_color(renderer_, color);
    SDL_RenderClear(renderer_);
}

void Renderer::draw_filled_rect(const Rect& rect, config::Color color) {
    if (!renderer_) return;
    const SDL_FRect sdl_rect{ rect.x, rect.y, rect.w, rect.h };
    set_draw_color(renderer_, color);
    SDL_RenderFillRect(renderer_, &sdl_rect);
}

void Renderer::draw_rect_outline(const Rect& rect, config::Color color) {
    if (!renderer_) return;
    const SDL_FRect sdl_rect{ rect.x, rect.y, rect.w, rect.h };
    set_draw_color(renderer_, color);
    SDL_RenderRect(renderer_, &sdl_rect);
}

void Renderer::present() {
    if (!renderer_) return;
    SDL_RenderPresent(renderer_);
}

} // namespace ember