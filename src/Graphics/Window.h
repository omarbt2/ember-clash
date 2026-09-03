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

    // Toggle the OS-level fullscreen state. Returns the new state
    // (true = fullscreen) or false if the call failed (e.g. SDL
    // could not acquire a fullscreen mode). The renderer's logical
    // presentation is unchanged because we use logical w/h, not
    // pixel w/h, so a display-mode switch is transparent to the
    // rest of the game.
    bool set_fullscreen(bool on);

    // The window's current fullscreen state. Tracked here (not
    // queried from SDL every frame) so tests can assert it
    // without an SDL_GetWindowFlags call.
    bool fullscreen() const { return fullscreen_; }

private:
    SDL_Window* window_{nullptr};
    int         width_{0};
    int         height_{0};
    bool        fullscreen_{false};
};

} // namespace ember