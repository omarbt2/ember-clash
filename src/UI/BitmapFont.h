#pragma once

#include "Core/Config.h"
#include "Graphics/Renderer.h"

// -----------------------------------------------------------------------------
// BitmapFont: a 4x6 monospace pixel font drawn with filled rects.
//
// We avoid SDL_ttf / FreeType / any asset pipeline on purpose. The font is
// small (the bitmask for every character fits in 24 bits), and the renderer
// already supports draw_filled_rect. This keeps the build dependency-free and
// the test harness independent of font availability.
//
// Glyph table conventions:
//   * Each glyph is 4 pixels wide and 6 rows tall.
//   * `bits[0..5]` is the 6 rows from top to bottom.
//   * The low 4 bits of each row are the pixels from left to right.
//   * Bit 0 of bits[0] is the top-left pixel.
//   * Glyphs for characters we don't render (anything outside the supported
//     set) are drawn as a 4x6 outlined rectangle ("?") by the helper.
//
// Supported characters: A-Z, 0-9, space, !, ?, -, :, ., %. Lowercase is
// uppercased before lookup. This is enough for the entire M5 HUD: damage
// numbers ("47%"), stock pips caption, "GAME!", "WINS", "PRESS R TO REMATCH".
// -----------------------------------------------------------------------------
namespace ember::ui {

// Cell size in render-space pixels. Each glyph is kCellPx x kCellPx * 1.5
// (4 cells wide, 6 rows tall) so a damage number like "147%" is about
// (4*4 + 4*3) * cell = 28 cells = 56 px wide at the default scale.
inline constexpr int kCellPx = 3;

inline constexpr int kGlyphW  = 4 * kCellPx;   // 12
inline constexpr int kGlyphH  = 6 * kCellPx;   // 18
inline constexpr int kAdvance = 5 * kCellPx;   // 15 (1 px gap)

namespace detail {

// Bits laid out top-to-bottom, low 4 bits of each byte are the row pixels
// left-to-right. Defined in anonymous-namespace-style .cpp so the static
// table doesn't bloat every translation unit. (Declared here, defined in
// the single .cpp we ship.)
struct GlyphBits {
    std::uint8_t rows[6];
};

const GlyphBits& glyph_for(char c);

// Draw a single glyph at (x, y) using the supplied color. The cell size is
// kCellPx; (x, y) is the top-left of the glyph's 4x6 cell region.
void draw_glyph(Renderer& r, int x, int y, char c, config::Color color);

} // namespace detail

// Draws a null-terminated string with the supplied top-left coordinate.
// Lowercase is uppercased; unsupported characters render as a "?". Newlines
// are not supported (callers wrap themselves).
inline void draw_text(Renderer& r, int x, int y, const char* s,
                      config::Color color = config::kHudTextColor) {
    if (!s) return;
    int cx = x;
    for (; *s; ++s) {
        detail::draw_glyph(r, cx, y, *s, color);
        cx += kAdvance;
    }
}

// Returns the width in render-space pixels of a string drawn with draw_text.
inline int measure_text(const char* s) {
    if (!s) return 0;
    int n = 0;
    for (; *s; ++s) ++n;
    return n * kAdvance - kCellPx;  // last glyph has no trailing gap
}

} // namespace ember::ui
