#include "UI/BitmapFont.h"

#include "Graphics/Renderer.h"

namespace ember::ui::detail {

namespace {

// "?" fallback glyph for characters we don't have in the table.
constexpr GlyphBits kQuestion{ .rows = {
    0b1110,
    0b0001,
    0b0010,
    0b0100,
    0b0000,
    0b0100,
} };

constexpr GlyphBits kEmpty{ .rows = { 0, 0, 0, 0, 0, 0 } };

// A-Z, 0-9, plus a small punctuation set. Each row is 4 pixels wide.
constexpr std::pair<char, GlyphBits> kGlyphs[] = {
    // Digits
    { '0', { .rows = { 0b0110, 0b1001, 0b1001, 0b1001, 0b1001, 0b0110 } } },
    { '1', { .rows = { 0b0010, 0b0110, 0b0010, 0b0010, 0b0010, 0b1110 } } },
    { '2', { .rows = { 0b1110, 0b0001, 0b0001, 0b1110, 0b1000, 0b1111 } } },
    { '3', { .rows = { 0b1110, 0b0001, 0b0110, 0b0001, 0b0001, 0b1110 } } },
    { '4', { .rows = { 0b1001, 0b1001, 0b1001, 0b1111, 0b0001, 0b0001 } } },
    { '5', { .rows = { 0b1111, 0b1000, 0b1110, 0b0001, 0b0001, 0b1110 } } },
    { '6', { .rows = { 0b0110, 0b1000, 0b1000, 0b1110, 0b1001, 0b0110 } } },
    { '7', { .rows = { 0b1111, 0b0001, 0b0010, 0b0100, 0b1000, 0b1000 } } },
    { '8', { .rows = { 0b0110, 0b1001, 0b0110, 0b1001, 0b1001, 0b0110 } } },
    { '9', { .rows = { 0b0110, 0b1001, 0b1001, 0b0111, 0b0001, 0b0110 } } },

    // Uppercase letters
    { 'A', { .rows = { 0b0110, 0b1001, 0b1001, 0b1111, 0b1001, 0b1001 } } },
    { 'B', { .rows = { 0b1110, 0b1001, 0b1110, 0b1001, 0b1001, 0b1110 } } },
    { 'C', { .rows = { 0b0111, 0b1000, 0b1000, 0b1000, 0b1000, 0b0111 } } },
    { 'D', { .rows = { 0b1110, 0b1001, 0b1001, 0b1001, 0b1001, 0b1110 } } },
    { 'E', { .rows = { 0b1111, 0b1000, 0b1110, 0b1000, 0b1000, 0b1111 } } },
    { 'F', { .rows = { 0b1111, 0b1000, 0b1110, 0b1000, 0b1000, 0b1000 } } },
    { 'G', { .rows = { 0b0111, 0b1000, 0b1011, 0b1001, 0b1001, 0b0111 } } },
    { 'H', { .rows = { 0b1001, 0b1001, 0b1001, 0b1111, 0b1001, 0b1001 } } },
    { 'I', { .rows = { 0b1110, 0b0100, 0b0100, 0b0100, 0b0100, 0b1110 } } },
    { 'J', { .rows = { 0b0011, 0b0001, 0b0001, 0b0001, 0b1001, 0b0110 } } },
    { 'K', { .rows = { 0b1001, 0b1010, 0b1100, 0b1100, 0b1010, 0b1001 } } },
    { 'L', { .rows = { 0b1000, 0b1000, 0b1000, 0b1000, 0b1000, 0b1111 } } },
    { 'M', { .rows = { 0b1001, 0b1111, 0b1111, 0b1001, 0b1001, 0b1001 } } },
    { 'N', { .rows = { 0b1001, 0b1101, 0b1111, 0b1011, 0b1001, 0b1001 } } },
    { 'O', { .rows = { 0b0110, 0b1001, 0b1001, 0b1001, 0b1001, 0b0110 } } },
    { 'P', { .rows = { 0b1110, 0b1001, 0b1001, 0b1110, 0b1000, 0b1000 } } },
    { 'Q', { .rows = { 0b0110, 0b1001, 0b1001, 0b1001, 0b1010, 0b0111 } } },
    { 'R', { .rows = { 0b1110, 0b1001, 0b1001, 0b1110, 0b1010, 0b1001 } } },
    { 'S', { .rows = { 0b0111, 0b1000, 0b0110, 0b0001, 0b0001, 0b1110 } } },
    { 'T', { .rows = { 0b1111, 0b0100, 0b0100, 0b0100, 0b0100, 0b0100 } } },
    { 'U', { .rows = { 0b1001, 0b1001, 0b1001, 0b1001, 0b1001, 0b0110 } } },
    { 'V', { .rows = { 0b1001, 0b1001, 0b1001, 0b1001, 0b0110, 0b0010 } } },
    { 'W', { .rows = { 0b1001, 0b1001, 0b1001, 0b1111, 0b1001, 0b1001 } } },
    { 'X', { .rows = { 0b1001, 0b1001, 0b0100, 0b0100, 0b1001, 0b1001 } } },
    { 'Y', { .rows = { 0b1001, 0b1001, 0b0100, 0b0100, 0b0100, 0b0100 } } },
    { 'Z', { .rows = { 0b1111, 0b0001, 0b0010, 0b0100, 0b1000, 0b1111 } } },

    // Punctuation
    { ' ', kEmpty },
    { '!', { .rows = { 0b0100, 0b0100, 0b0100, 0b0100, 0b0000, 0b0100 } } },
    { '?', kQuestion },
    { '+', { .rows = { 0b0000, 0b0100, 0b0100, 0b1111, 0b0100, 0b0100 } } },
    { '-', { .rows = { 0b0000, 0b0000, 0b0000, 0b1111, 0b0000, 0b0000 } } },
    { ':', { .rows = { 0b0000, 0b0100, 0b0000, 0b0000, 0b0100, 0b0000 } } },
    { '.', { .rows = { 0b0000, 0b0000, 0b0000, 0b0000, 0b0000, 0b0100 } } },
    { '%', { .rows = { 0b1100, 0b1100, 0b0010, 0b0100, 0b0011, 0b0011 } } },
};

} // namespace

const GlyphBits& glyph_for(char c) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    for (const auto& kv : kGlyphs) {
        if (kv.first == c) return kv.second;
    }
    return kQuestion;
}

void draw_glyph(Renderer& r, int x, int y, char c, config::Color color) {
    const GlyphBits& g = glyph_for(c);
    for (int row = 0; row < 6; ++row) {
        const std::uint8_t bits = g.rows[row];
        for (int col = 0; col < 4; ++col) {
            if ((bits >> (3 - col)) & 1) {
                r.draw_filled_rect(
                    { static_cast<float>(x + col * kCellPx),
                      static_cast<float>(y + row * kCellPx),
                      static_cast<float>(kCellPx),
                      static_cast<float>(kCellPx) },
                    color);
            }
        }
    }
}

} // namespace ember::ui::detail
