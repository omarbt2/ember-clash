#pragma once

#include "Core/Config.h"

#include <array>
#include <cstdint>
#include <string_view>

struct SDL_Renderer;
struct SDL_Texture;

namespace ember {

class Sprite;
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

    // Blits a Sprite (procedural 64x64 RGBA) into the destination rect.
    // The texture is uploaded lazily and cached by Sprite identity so the
    // cost is amortized to a single SDL_UpdateTexture per Sprite per App
    // lifetime. The flip parameter mirrors the sprite horizontally when
    // true (used by Fighter to face the opponent).
    void draw_sprite(const Sprite& sprite,
                     const Rect& dst,
                     int pixel_size,
                     bool flip_horizontal);

    // Fills the back buffer with a vertical gradient (top -> bottom).
    // M6-A: used by Arena::render() to give the background a soft
    // depth tint without an asset.
    void draw_vertical_gradient(const Rect& rect,
                                config::Color top,
                                config::Color bottom);

    // M6-C: set a per-frame view offset (render pixels) that every
    // subsequent draw call subtracts from its destination rect. Used by
    // App to implement the post-KO screen shake. (0, 0) is the identity
    // offset and is the default at construction.
    void set_view_offset(float dx, float dy);

    // Render-scale multiplier. 1.0 = identity (1:1 logical to window).
    // 2.0 = each logical pixel becomes 2x2 device pixels. We
    // implement this via SDL_RenderSetScale so the rest of the
    // rendering pipeline is unchanged. Render scale is set at the
    // App level whenever settings.render_scale changes; a value of
    // 1 means the game's internal resolution is rendered 1:1.
    void set_render_scale(float s);
    float render_scale() const { return render_scale_; }

    // Presents the current back buffer.
    void present();

    // Sprite texture cache. Public for implementation reasons: a free
    // helper in Renderer.cpp (find_slot) needs to refer to the slot type
    // and the slot count, and a free function in an anonymous namespace
    // cannot reach private members without a friend declaration. Making
    // the cache type public is harmless - it is an internal layout
    // detail that callers should never touch.
    static constexpr int kSpriteCacheSlots = 4;
    struct SpriteCacheSlot {
        const Sprite* key{nullptr};
        SDL_Texture*  texture{nullptr};
        // M6-B: track which frame the texture was last uploaded with so
        // we can re-upload when the sprite's current_frame changes.
        // -1 == "no frame uploaded yet".
        int last_frame{-1};
    };

private:
    SDL_Renderer* renderer_{nullptr};
    int           out_w_{0};
    int           out_h_{0};
    std::array<SpriteCacheSlot, kSpriteCacheSlots> sprite_cache_{};
    // M6-C: per-frame view offset. Defaults to (0, 0) so callers that
    // never touch it (e.g. the test harnesses) get the unmodified output.
    float         view_offset_x_{0.0f};
    float         view_offset_y_{0.0f};
    float         render_scale_{1.0f};
};

} // namespace ember