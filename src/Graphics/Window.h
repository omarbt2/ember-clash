#pragma once

#include <cstdint>
#include <string>

struct SDL_Window;

namespace ember {

// Thin RAII wrapper around SDL_Window. Owns the native handle and frees it
// in the destructor.
class Window {
public:
    Window(std::string title, int width, int height);
    ~Window();

    Window(const Window&)            = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    SDL_Window* native() const { return window_; }

    int  width()  const { return width_;  }
    int  height() const { return height_; }

    bool is_valid() const { return window_ != nullptr; }

private:
    SDL_Window* window_{nullptr};
    int         width_{0};
    int         height_{0};
};

} // namespace ember