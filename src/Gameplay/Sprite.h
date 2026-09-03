#pragma once

#include "Gameplay/ArtConfig.h"

#include <array>
#include <cstdint>

namespace ember {

// =============================================================================
// Sprite — a procedurally-generated RGBA pixel buffer.
//
// The M6 fighter sprite is generated at startup from a fixed geometric
// silhouette (head, torso, arms, legs) and a FighterPalette. We do NOT
// load any asset file: this is the project's deliberate "no external
// art dependencies" rule, and it also means the test harness can verify
// the generated pixels without a filesystem.
//
// Pixel layout: kSize x kSize, top-down rows, 4 bytes per pixel
// (R, G, B, A). alpha=0 means "transparent" and the underlying arena
// shows through. The Renderer treats this buffer as read-only after
// construction; Fighter::render() just blits it. The silhouette is
// palette-indexed (see Index) so a single layout produces both P1
// (warm) and P2 (cool) fighters with no per-palette duplication.
// =============================================================================
class Sprite {
public:
    static constexpr int kSize = 64;

    // Index into the palette: 0 = transparent, 1 = outline,
    // 2 = body, 3 = accent, 4 = highlight. Encoding the silhouette in
    // terms of palette indices (not raw colours) lets one silhouette
    // produce both P1 (warm) and P2 (cool) fighters with no per-palette
    // duplication.
    enum class Index : std::uint8_t {
        Transparent = 0,
        Outline     = 1,
        Body        = 2,
        Accent      = 3,
        Highlight   = 4,
    };

    Sprite(const art::FighterPalette& palette);

    const std::uint8_t* pixels_rgba() const { return pixels_.data(); }
    int width()  const { return kSize; }
    int height() const { return kSize; }

    // How many pixels of the sprite are non-transparent. Used by tests to
    // sanity-check the silhouette generator (it should never produce a
    // blank sprite, and it should never produce a fully-opaque square
    // either - that would mean the silhouette became a box again).
    int opaque_pixel_count() const { return opaque_count_; }

    // Tests need a way to read the palette index for a given pixel so they
    // can assert "this pixel is the outline stop", "this pixel is the
    // body", etc. The public pixel buffer is RGBA so the visible colour
    // is lossless; the index map is the inverse of the palette lookup and
    // is what the test asserts on.
    Index index_at(int x, int y) const {
        return index_map_[static_cast<std::size_t>(y * kSize + x)];
    }

private:
    std::array<std::uint8_t, kSize * kSize * 4> pixels_{};
    std::array<Index,     kSize * kSize>       index_map_{};
    int opaque_count_{0};

    void set_pixel(int x, int y, Index idx, const art::FighterPalette& pal);
};

} // namespace ember
