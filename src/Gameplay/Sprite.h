#pragma once

#include "Gameplay/ArtConfig.h"

#include <array>
#include <cstdint>
#include <vector>

namespace ember {

// =============================================================================
// Sprite — a procedurally-generated RGBA pixel buffer with one or more
// animation frames.
//
// M6-A shipped a single-frame Sprite: one RGBA buffer, one index map, one
// opaque_count. M6-B adds a small "frame strip" so a Fighter can have an
// idle pose, a 2-frame walk cycle, a jump pose, an attack pose, and a
// hitstun pose - all sharing the same palette and silhouette geometry.
//
// The single-frame constructor is preserved so the M6-A test keeps
// passing; new multi-frame sprites are built via the static factory
// Sprite::build_animation_strip(palette, animation_kind). The Renderer
// sees the same surface: draw_sprite(sprite, rect, pixel_size, flip) -
// the renderer just notices the current frame changed and re-uploads
// the (cheap) 16 KB texture.
//
// All buffers are immutable after construction. M6-B's animation driver
// only swaps the "current frame" index; it never edits a frame in place.
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

    // -----------------------------------------------------------------
    // M6-B: which animation pose to draw. Each pose is one or more
    // frames stored consecutively in the Sprite's frame strip. Poses
    // are added to the strip in this enum's order, so a Sprite's
    // frame_count() == sum(frames_per_pose).
    // -----------------------------------------------------------------
    enum class AnimationKind {
        Idle    = 0,
        Walk    = 1,   // 2 frames
        Jump    = 2,
        Attack  = 3,
        Hitstun = 4,
        _Count  = 5,
    };

    // M6-A: single-frame Sprite (kept verbatim - the M6-A test still
    // expects this constructor to produce a 1-frame Sprite).
    explicit Sprite(const art::FighterPalette& palette);

    // M6-B: build a multi-frame fighter sprite for a palette. The
    // returned Sprite's frame strip is:
    //   [Idle: 1, Walk: 2, Jump: 1, Attack: 1, Hitstun: 1] = 6 frames.
    static Sprite build_animation_strip(const art::FighterPalette& palette);

    // The currently-displayed frame. The animation driver calls
    // set_current_frame() each tick.
    int current_frame() const { return current_frame_; }
    void set_current_frame(int f) {
        if (f < 0) f = 0;
        if (f >= static_cast<int>(frames_.size())) f = static_cast<int>(frames_.size()) - 1;
        current_frame_ = f;
    }
    int frame_count() const { return static_cast<int>(frames_.size()); }

    // Convenience: returns the AnimationKind whose first frame is the
    // current frame. Used by the renderer when it wants to log which
    // pose is showing (debug aid, not used in production paths).
    AnimationKind current_kind() const { return kind_of_frame(current_frame_); }

    // Direct read of the current frame's pixels (used by Renderer to
    // upload into an SDL_Texture). Returns a pointer to a contiguous
    // 4-byte-per-pixel RGBA buffer of size kSize * kSize * 4.
    const std::uint8_t* pixels_rgba() const { return frames_[current_frame_].pixels.data(); }

    int width()  const { return kSize; }
    int height() const { return kSize; }

    // How many pixels of the CURRENT frame are non-transparent. Tests
    // use this to sanity-check pose generators.
    int opaque_pixel_count() const { return frames_[current_frame_].opaque_count; }

    // Same as opaque_pixel_count() but summed over every frame in the
    // strip. Tests use this to confirm the walk cycle is a real
    // 2-frame cycle (the two walk frames differ in opaque count
    // because the legs swing).
    int total_opaque_pixel_count() const {
        int sum = 0;
        for (const Frame& f : frames_) sum += f.opaque_count;
        return sum;
    }

    // Index of the palette slot at (x, y) in the CURRENT frame.
    Index index_at(int x, int y) const {
        return frames_[current_frame_].index_map[static_cast<std::size_t>(y * kSize + x)];
    }

    // Per-frame variant for tests that want to compare two specific
    // frames in the same Sprite.
    Index index_at(int frame, int x, int y) const {
        if (frame < 0 || frame >= static_cast<int>(frames_.size())) {
            return Index::Transparent;
        }
        return frames_[static_cast<std::size_t>(frame)].index_map[static_cast<std::size_t>(y * kSize + x)];
    }

    // The first frame index that belongs to a given AnimationKind. Use
    // with set_current_frame to jump to a specific pose. The bounds are
    // validated by static_assert in the .cpp.
    static int first_frame_of(AnimationKind k) { return kFrameOffsets[static_cast<int>(k)]; }

    // Which kind a given global frame index belongs to. Walks the
    // kFrameOffsets table; O(1) because there are only a few poses.
    static AnimationKind kind_of_frame(int global_frame) {
        for (int k = static_cast<int>(AnimationKind::_Count) - 1; k >= 0; --k) {
            if (global_frame >= kFrameOffsets[k]) return static_cast<AnimationKind>(k);
        }
        return AnimationKind::Idle;
    }

private:
    struct Frame {
        std::array<std::uint8_t, kSize * kSize * 4> pixels{};
        std::array<Index,     kSize * kSize>       index_map{};
        int opaque_count{0};
    };

    std::vector<Frame> frames_;
    int current_frame_{0};

    // Cumulative frame offsets for each AnimationKind. The .cpp defines
    // this so the offsets stay next to the pose generators that build
    // each block of frames.
    static const int kFrameOffsets[static_cast<int>(AnimationKind::_Count)];

    void set_pixel(int frame_index, int x, int y, Index idx, const art::FighterPalette& pal);
};

} // namespace ember
