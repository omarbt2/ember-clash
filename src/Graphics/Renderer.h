#pragma once

#include "Core/Config.h"

#include <cstdint>
#include <string_view>

struct SDL_Renderer;

namespace ember {

class Window;

// Minimal 2D renderer used during the foundation milestone. Real textures,
// cameras, and shaders will be layered on top later.
class Renderer {
public:
    struct Rect {
        float x;
        float y;
        float w;
        float h;
    };

    Renderer(Window& window);
    ~Renderer();

    Renderer(const Renderer&)            = delete;
    Renderer& operator=(const Renderer&) = delete;

    SDL_Renderer* native() const { return renderer_; }

    int output_width()  const { return out_w_; }
    int output_height() const { return out_h_; }

    // Clears the back buffer to the supplied color.
    void clear(config::Color color);

    // Draws a solid rectangle in render-space coordinates.
    void draw_filled_rect(const Rect& rect, config::Color color);

    // Draws a 1-pixel-thick rectangle outline.
    void draw_rect_outline(const Rect& rect, config::Color color);

    // Presents the current back buffer.
    void present();

private:
    SDL_Renderer* renderer_{nullptr};
    int           out_w_{0};
    int           out_h_{0};
};

} // namespace ember