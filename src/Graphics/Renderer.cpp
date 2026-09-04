#include "Graphics/Renderer.h"

#include "Core/Logger.h"
#include "Gameplay/Sprite.h"
#include "Graphics/Window.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>

#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>

namespace ember {

namespace {

void set_draw_color(SDL_Renderer* r, config::Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

// Picks the cache slot to use for the given Sprite. We try, in order:
//   1) an existing entry that matches the key (return it for update),
//   2) an empty slot (return it to fill),
//   3) slot 0 as a last-resort eviction target. With kSpriteCacheSlots
//      = 4 and only two sprites (P1 + P2) in practice, paths 1 and 2
//      cover the steady state.
Renderer::SpriteCacheSlot* find_slot(
        std::array<Renderer::SpriteCacheSlot,
                   Renderer::kSpriteCacheSlots>& cache,
        const Sprite* key) {
    for (auto& entry : cache) {
        if (entry.key == key) return &entry;
    }
    for (auto& entry : cache) {
        if (entry.key == nullptr) return &entry;
    }
    return &cache[0];
}

} // namespace

Renderer::Renderer(Window& window) {
    SDL_Window* raw_window = window.native();
    if (!raw_window) {
        Logger::error("Renderer: window is invalid.");
        return;
    }

    // P17: request vsync at renderer creation. The accumulator
    // in App::run() is the authoritative cap, but vsync is a
    // useful belt-and-suspenders: it keeps the GPU from blasting
    // 500 FPS through the present queue, and on a 60 Hz display
    // it makes the frame cap implicit so the accumulator never
    // has to do work. On a 144/240 Hz display the accumulator
    // still runs the simulation at exactly 60 Hz while the
    // display shows 144+ frames per second of static state.
    //
    // SDL3 changed the API: the old SDL_RENDERER_PRESENTVSYNC
    // creation flag was removed; vsync is now a per-renderer
    // property toggled by SDL_SetRenderVSync(1). 1 means
    // "sync to display refresh".
    SDL_Renderer* raw = SDL_CreateRenderer(raw_window, nullptr);
    if (!raw) {
        Logger::error(std::string{"SDL_CreateRenderer failed: "} + SDL_GetError());
        return;
    }
    if (!SDL_SetRenderVSync(raw, 1)) {
        // Not fatal: the accumulator in App::run() still caps
        // the simulation rate, and some displays simply do not
        // support adaptive vsync. We log it and continue.
        Logger::warn(std::string{"SDL_SetRenderVSync: "} + SDL_GetError());
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
    // Free any cached sprite textures first; they were created with this
    // renderer's SDL_CreateTexture, so they must be released while the
    // renderer is still alive.
    for (auto& entry : sprite_cache_) {
        if (entry.texture) {
            SDL_DestroyTexture(entry.texture);
            entry.texture = nullptr;
        }
        entry.key       = nullptr;
        entry.last_frame = -1;
    }
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

void Renderer::set_view_offset(float dx, float dy) {
    view_offset_x_ = dx;
    view_offset_y_ = dy;
}

void Renderer::set_render_scale(float s) {
    // Clamp to a sane range. Render scale of 0 would be a divide-
    // by-zero; very large scales are pointless and break the
    // window. Settings stores an integer 1..N; we just round
    // here so the int->float conversion is lossless.
    if (s < 0.5f) s = 0.5f;
    if (s > 4.0f) s = 4.0f;
    if (s == render_scale_) {
        return;
    }
    render_scale_ = s;
    if (renderer_) {
        SDL_SetRenderScale(renderer_, s, s);
    }
}

namespace {
// M6-C: apply the per-frame view offset to a logical rect before we hand
// it to SDL. Subtracting the offset makes a positive offset "push the
// camera in the negative direction" - the conventional feel for an
// impact (the player is reading the scene, not the camera, and the
// scene wobbling around the impact point is what reads as weight).
SDL_FRect with_offset(const Renderer::Rect& r, float ox, float oy) {
    return SDL_FRect{ r.x - ox, r.y - oy, r.w, r.h };
}
} // namespace

void Renderer::draw_filled_rect(const Rect& rect, config::Color color) {
    if (!renderer_) return;
    const SDL_FRect sdl_rect = with_offset(rect, view_offset_x_, view_offset_y_);
    set_draw_color(renderer_, color);
    SDL_RenderFillRect(renderer_, &sdl_rect);
}

void Renderer::draw_rect_outline(const Rect& rect, config::Color color) {
    if (!renderer_) return;
    const SDL_FRect sdl_rect = with_offset(rect, view_offset_x_, view_offset_y_);
    set_draw_color(renderer_, color);
    SDL_RenderRect(renderer_, &sdl_rect);
}

void Renderer::draw_sprite(const Sprite& sprite,
                           const Rect& dst,
                           int pixel_size,
                           bool flip_horizontal) {
    if (!renderer_) return;
    if (pixel_size <= 0) return;

    auto* slot = find_slot(sprite_cache_, &sprite);

    // M6-B: a Sprite's pixels change when its current_frame changes (the
    // animation driver flips through the strip). We cache the texture
    // by Sprite* identity and re-upload only when the frame index
    // changes - so walking fighters pay exactly one extra
    // SDL_UpdateTexture per walk-step instead of every render.
    if (slot->key != &sprite) {
        // New slot / eviction: drop any previous texture and build a
        // fresh one for this Sprite.
        if (slot->texture) {
            SDL_DestroyTexture(slot->texture);
            slot->texture = nullptr;
        }
        SDL_Texture* tex = SDL_CreateTexture(
            renderer_,
            SDL_PIXELFORMAT_RGBA32,
            SDL_TEXTUREACCESS_STATIC,
            Sprite::kSize,
            Sprite::kSize);
        if (!tex) {
            Logger::warn(std::string{"SDL_CreateTexture: "} + SDL_GetError());
            return;
        }
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        if (!SDL_UpdateTexture(tex, nullptr,
                              sprite.pixels_rgba(),
                              Sprite::kSize * 4)) {
            Logger::warn(std::string{"SDL_UpdateTexture: "} + SDL_GetError());
            SDL_DestroyTexture(tex);
            return;
        }
        slot->key        = &sprite;
        slot->texture    = tex;
        slot->last_frame = sprite.current_frame();
    } else if (slot->last_frame != sprite.current_frame()) {
        // Same Sprite, different frame: re-upload the current pixels
        // into the existing texture. 16 KB per call, cheap.
        if (slot->texture) {
            if (!SDL_UpdateTexture(slot->texture, nullptr,
                                   sprite.pixels_rgba(),
                                   Sprite::kSize * 4)) {
                Logger::warn(std::string{"SDL_UpdateTexture: "} + SDL_GetError());
            }
            slot->last_frame = sprite.current_frame();
        }
    }

    SDL_FRect dst_rect{
        dst.x - view_offset_x_, dst.y - view_offset_y_,
        static_cast<float>(Sprite::kSize * pixel_size),
        static_cast<float>(Sprite::kSize * pixel_size),
    };
    if (flip_horizontal) {
        SDL_RenderTextureRotated(renderer_, slot->texture, nullptr, &dst_rect,
                                 0.0, nullptr, SDL_FLIP_HORIZONTAL);
    } else {
        SDL_RenderTexture(renderer_, slot->texture, nullptr, &dst_rect);
    }
}

void Renderer::draw_vertical_gradient(const Rect& rect,
                                     config::Color top,
                                     config::Color bottom) {
    if (!renderer_) return;
    // Approximate a smooth gradient with a stack of 1-px-tall horizontal
    // bands. Cheap, deterministic, no shaders required. We choose a band
    // height of 2 px for a small-but-not-perceptible dither; SDL's
    // scaling to the window is the dominant cost anyway.
    const float h = rect.h;
    if (h <= 0.0f) return;
    const int bands = std::max(1, static_cast<int>(h / 2.0f));
    const float step = h / static_cast<float>(bands);
    for (int i = 0; i < bands; ++i) {
        const float t0 = static_cast<float>(i)     / static_cast<float>(bands);
        const float t1 = static_cast<float>(i + 1) / static_cast<float>(bands);
        const float tm = 0.5f * (t0 + t1);
        const auto lerp = [tm](std::uint8_t a, std::uint8_t b) {
            return static_cast<std::uint8_t>(
                static_cast<int>(a) +
                static_cast<int>(static_cast<float>(b) - static_cast<float>(a)) * tm);
        };
        config::Color c{
            lerp(top.r, bottom.r),
            lerp(top.g, bottom.g),
            lerp(top.b, bottom.b),
            lerp(top.a, bottom.a),
        };
        const SDL_FRect band{ rect.x - view_offset_x_,
                              rect.y + step * static_cast<float>(i) - view_offset_y_,
                              rect.w,
                              step + 0.5f };
        set_draw_color(renderer_, c);
        SDL_RenderFillRect(renderer_, &band);
    }
}

void Renderer::present() {
    if (!renderer_) return;
    SDL_RenderPresent(renderer_);
}

} // namespace ember