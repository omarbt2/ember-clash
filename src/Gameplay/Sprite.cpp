#include "Gameplay/Sprite.h"

#include <algorithm>

namespace ember {

namespace {

// -----------------------------------------------------------------------------
// Primitive drawing helpers. All operate on a Sprite's pixel + index maps.
// We avoid a separate canvas type because the sprite is 64x64, and the
// overhead of a second buffer would dwarf the work.
// -----------------------------------------------------------------------------
void put(Sprite::Index* idx, std::uint8_t* rgba,
         int x, int y, Sprite::Index i,
         const art::FighterPalette& pal) {
    if (x < 0 || x >= Sprite::kSize || y < 0 || y >= Sprite::kSize) return;
    const std::size_t pix = static_cast<std::size_t>((y * Sprite::kSize + x) * 4);
    const std::size_t im  = static_cast<std::size_t>(y * Sprite::kSize + x);
    idx[im] = i;
    config::Color c{};
    switch (i) {
        case Sprite::Index::Outline:   c = pal.outline;   break;
        case Sprite::Index::Body:      c = pal.body;      break;
        case Sprite::Index::Accent:    c = pal.accent;    break;
        case Sprite::Index::Highlight: c = pal.highlight; break;
        case Sprite::Index::Transparent: c = {0, 0, 0, 0}; break;
    }
    rgba[pix + 0] = c.r;
    rgba[pix + 1] = c.g;
    rgba[pix + 2] = c.b;
    rgba[pix + 3] = c.a;
}

void fill_rect(Sprite::Index* idx, std::uint8_t* rgba,
               int x, int y, int w, int h, Sprite::Index i,
               const art::FighterPalette& pal) {
    for (int yy = y; yy < y + h; ++yy) {
        for (int xx = x; xx < x + w; ++xx) {
            put(idx, rgba, xx, yy, i, pal);
        }
    }
}

void outline_rect(Sprite::Index* idx, std::uint8_t* rgba,
                  int x, int y, int w, int h, Sprite::Index i,
                  const art::FighterPalette& pal) {
    for (int xx = x; xx < x + w; ++xx) {
        put(idx, rgba, xx, y,         i, pal);
        put(idx, rgba, xx, y + h - 1, i, pal);
    }
    for (int yy = y; yy < y + h; ++yy) {
        put(idx, rgba, x,         yy, i, pal);
        put(idx, rgba, x + w - 1, yy, i, pal);
    }
}

} // namespace

Sprite::Sprite(const art::FighterPalette& palette) {
    pixels_.fill(0);
    index_map_.fill(Index::Transparent);

    // ---- Layout (top-down) -----------------------------------------------
    // 64x64 grid. We draw a chunky cartoon fighter: round head, broad
    // shoulders / torso, arms at the side, legs that taper to a point.
    // Coordinate origin is top-left; y=0 is the top of the sprite.
    //
    // The fighter body is centred horizontally; there's 8 px of transparent
    // margin on each side (so the silhouette can extend without touching
    // the texture edge in M6-B when we draw attack frames).
    //
    //   y in 0..7     transparent
    //   y in 8..27    head (round; outline + body + highlight cap)
    //   y in 28..47   torso (rectangle with accent sash + highlight stripe)
    //   y in 48..55   arms (small bars at the sides)
    //   y in 56..63   legs (two rectangles separated by a gap)
    //
    // Numbers below are source pixels (Sprite::kSize=64). The Renderer
    // scales by art::kRenderPixelSize so a 48x64 body becomes 96x128 on
    // screen.

    // Head: 20x20, centred horizontally (x = 22..41), y = 8..27.
    // We use outline + body + a small highlight block on the top-left of
    // the head so the silhouette has a clear "light source" - cheap but
    // effective read.
    outline_rect(index_map_.data(), pixels_.data(), 22, 8, 20, 20,
                 Index::Outline, palette);
    fill_rect   (index_map_.data(), pixels_.data(), 23, 9, 18, 18,
                 Index::Body, palette);
    // Eyes - two small dark slots so the face reads. Outline stop is the
    // same colour as the body outline, so this stays in palette.
    put(index_map_.data(), pixels_.data(), 28, 16, Index::Outline, palette);
    put(index_map_.data(), pixels_.data(), 35, 16, Index::Outline, palette);
    // Highlight cap on top of the head.
    fill_rect   (index_map_.data(), pixels_.data(), 24, 9, 16, 3,
                 Index::Highlight, palette);
    // Accent stripe at the bottom of the head (a "headband" / "crown").
    fill_rect   (index_map_.data(), pixels_.data(), 23, 23, 18, 2,
                 Index::Accent, palette);

    // Torso: 28x20, centred (x = 18..45), y = 28..47. Outline + body +
    // a vertical accent sash and a horizontal highlight stripe.
    outline_rect(index_map_.data(), pixels_.data(), 18, 28, 28, 20,
                 Index::Outline, palette);
    fill_rect   (index_map_.data(), pixels_.data(), 19, 29, 26, 18,
                 Index::Body, palette);
    // Accent sash (vertical centre stripe).
    fill_rect   (index_map_.data(), pixels_.data(), 30, 30, 4, 16,
                 Index::Accent, palette);
    // Highlight stripe on the upper-left "shoulder".
    fill_rect   (index_map_.data(), pixels_.data(), 20, 30, 6, 3,
                 Index::Highlight, palette);

    // Arms: two 4x20 bars at the sides of the torso, slightly below the
    // shoulder. Outline + body. They sit at x = 14..17 and 46..49,
    // y = 32..51 (so they extend just past the torso for a thicker look).
    outline_rect(index_map_.data(), pixels_.data(), 14, 32, 4, 20,
                 Index::Outline, palette);
    fill_rect   (index_map_.data(), pixels_.data(), 15, 33, 2, 18,
                 Index::Body, palette);
    outline_rect(index_map_.data(), pixels_.data(), 46, 32, 4, 20,
                 Index::Outline, palette);
    fill_rect   (index_map_.data(), pixels_.data(), 47, 33, 2, 18,
                 Index::Body, palette);

    // Legs: two 8x8 rectangles at the bottom, with a 4 px gap between
    // them. Outline + body. y = 48..55, x = 22..29 (left leg) and
    // 35..42 (right leg).
    outline_rect(index_map_.data(), pixels_.data(), 22, 48, 8, 8,
                 Index::Outline, palette);
    fill_rect   (index_map_.data(), pixels_.data(), 23, 49, 6, 6,
                 Index::Body, palette);
    outline_rect(index_map_.data(), pixels_.data(), 34, 48, 8, 8,
                 Index::Outline, palette);
    fill_rect   (index_map_.data(), pixels_.data(), 35, 49, 6, 6,
                 Index::Body, palette);
    // Feet: one-row accent line at the bottom of each leg.
    fill_rect   (index_map_.data(), pixels_.data(), 23, 55, 6, 1,
                 Index::Accent, palette);
    fill_rect   (index_map_.data(), pixels_.data(), 35, 55, 6, 1,
                 Index::Accent, palette);

    // Count opaque pixels. This is what the test harness uses to confirm
    // the silhouette is "filled" without having to scan the buffer.
    int count = 0;
    for (Index i : index_map_) {
        if (i != Index::Transparent) ++count;
    }
    opaque_count_ = count;
}

void Sprite::set_pixel(int x, int y, Index idx, const art::FighterPalette& pal) {
    put(index_map_.data(), pixels_.data(), x, y, idx, pal);
}

} // namespace ember
