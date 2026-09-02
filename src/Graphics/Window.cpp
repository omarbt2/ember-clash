#include "Graphics/Window.h"

#include "Core/Logger.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

#include <utility>

namespace ember {

Window::Window(std::string title, int width, int height)
    : width_(width), height_(height)
{
    SDL_Window* raw = SDL_CreateWindow(
        title.c_str(),
        static_cast<int>(width_),
        static_cast<int>(height_),
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );

    if (!raw) {
        Logger::error(std::string{"SDL_CreateWindow failed: "} + SDL_GetError());
        return;
    }

    window_ = raw;
}

Window::~Window() {
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
}

Window::Window(Window&& other) noexcept
    : window_(other.window_), width_(other.width_), height_(other.height_)
{
    other.window_ = nullptr;
    other.width_  = 0;
    other.height_ = 0;
}

Window& Window::operator=(Window&& other) noexcept {
    if (this != &other) {
        if (window_) SDL_DestroyWindow(window_);

        window_       = other.window_;
        width_        = other.width_;
        height_       = other.height_;
        other.window_ = nullptr;
        other.width_  = 0;
        other.height_ = 0;
    }
    return *this;
}

} // namespace ember