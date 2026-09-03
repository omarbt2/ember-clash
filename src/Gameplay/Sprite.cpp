#include "Gameplay/Sprite.h"

#include <algorithm>
#include <cstring>

namespace ember {

namespace {

// -----------------------------------------------------------------------------
// Primitive drawing helpers. All operate on a Sprite::Frame's pixel + index
// maps. We avoid a separate canvas type because the sprite is 64x64, and
// the overhead of a second buffer would dwarf the work.
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
               int x0, int y0, int w, int h,
               Sprite::Index fill,
               const art::FighterPalette& pal) {
    for (int y = y0; y < y0 + h; ++y) {
        for (int x = x0; x < x0 + w; ++x) {
            put(idx, rgba, x, y, fill, pal);
        }
    }
}

void outline_rect(Sprite::Index* idx, std::uint8_t* rgba,
                  int x0, int y0, int w, int h,
                  Sprite::Index outline,
                  const art::FighterPalette& pal) {
    for (int x = x0; x < x0 + w; ++x) {
        put(idx, rgba, x, y0,         outline, pal);
        put(idx, rgba, x, y0 + h - 1, outline, pal);
    }
    for (int y = y0; y < y0 + h; ++y) {
        put(idx, rgba, x0,         y, outline, pal);
        put(idx, rgba, x0 + w - 1, y, outline, pal);
    }
}

int count_opaque(const Sprite::Index* idx) {
    int n = 0;
    for (std::size_t i = 0; i < Sprite::kSize * Sprite::kSize; ++i) {
        if (idx[i] != Sprite::Index::Transparent) ++n;
    }
    return n;
}

// -----------------------------------------------------------------------------
// Shared silhouette geometry. The body region is always 48x64 source pixels
// (matches Fighter::kWidth / kHeight) with 8 px of margin on each side so
// the M6-B attack pose can extend an arm into the right margin.
// -----------------------------------------------------------------------------
constexpr int kBodyMargin = 8;
constexpr int kBodyBaseX  = kBodyMargin;
constexpr int kHeadX      = kBodyBaseX + 6;       // 14
constexpr int kHeadY      = 6;
constexpr int kHeadW      = 18;
constexpr int kHeadH      = 12;
constexpr int kTorsoX     = kBodyBaseX + 4;       // 12
constexpr int kTorsoY     = kHeadY + kHeadH;      // 18
constexpr int kTorsoW     = (Sprite::kSize - 2 * kBodyMargin) - 8;  // 40
constexpr int kTorsoH     = 16;
constexpr int kLegY       = kTorsoY + kTorsoH;   // 34
constexpr int kLegH       = 16;

} // namespace

// Frame offset table. Order must match the AnimationKind enum.
const int Sprite::kFrameOffsets[static_cast<int>(Sprite::AnimationKind::_Count)] = {
    0,  // Idle
    1,  // Walk (frame 0)
    3,  // Jump
    4,  // Attack
    5,  // Hitstun
};

// -----------------------------------------------------------------------------
// Pose generators. Each function draws one frame's worth of pixels into a
// freshly-emptied Frame. Idle is the M6-A silhouette verbatim; every other
// pose is a small variation.
// -----------------------------------------------------------------------------

static void draw_idle(Sprite::Index* idx, std::uint8_t* rgba,
                      const art::FighterPalette& pal) {
    outline_rect(idx, rgba, kHeadX, kHeadY, kHeadW, kHeadH, Sprite::Index::Outline, pal);
    fill_rect(idx, rgba, kHeadX + 1, kHeadY + 1, kHeadW - 2, kHeadH / 2 - 1, Sprite::Index::Highlight, pal);
    fill_rect(idx, rgba, kHeadX + 1, kHeadY + kHeadH / 2, kHeadW - 2, kHeadH / 2 - 1, Sprite::Index::Accent, pal);

    outline_rect(idx, rgba, kTorsoX, kTorsoY, kTorsoW, kTorsoH, Sprite::Index::Outline, pal);
    fill_rect(idx, rgba, kTorsoX + 1, kTorsoY + 1, kTorsoW - 2, 3, Sprite::Index::Accent, pal);
    fill_rect(idx, rgba, kTorsoX + 1, kTorsoY + 4, kTorsoW - 2, kTorsoH - 5, Sprite::Index::Body, pal);

    for (int side = 0; side < 2; ++side) {
        const int arm_x = (side == 0) ? kTorsoX - 2 : kTorsoX + kTorsoW - 1;
        fill_rect(idx, rgba, arm_x, kTorsoY + 1, 2, kTorsoH - 4, Sprite::Index::Body, pal);
    }

    outline_rect(idx, rgba, kTorsoX + 6, kLegY, 4, kLegH, Sprite::Index::Outline, pal);
    fill_rect(idx, rgba, kTorsoX + 7, kLegY + 1, 2, kLegH - 2, Sprite::Index::Body, pal);
    outline_rect(idx, rgba, kTorsoX + kTorsoW - 10, kLegY, 4, kLegH, Sprite::Index::Outline, pal);
    fill_rect(idx, rgba, kTorsoX + kTorsoW - 9, kLegY + 1, 2, kLegH - 2, Sprite::Index::Body, pal);
}

static void draw_walk(Sprite::Index* idx, std::uint8_t* rgba,
                      const art::FighterPalette& pal,
                      int walk_frame) {
    outline_rect(idx, rgba, kHeadX, kHeadY, kHeadW, kHeadH, Sprite::Index::Outline, pal);
    fill_rect(idx, rgba, kHeadX + 1, kHeadY + 1, kHeadW - 2, kHeadH / 2 - 1, Sprite::Index::Highlight, pal);
    fill_rect(idx, rgba, kHeadX + 1, kHeadY + kHeadH / 2, kHeadW - 2, kHeadH / 2 - 1, Sprite::Index::Accent, pal);

    outline_rect(idx, rgba, kTorsoX, kTorsoY, kTorsoW, kTorsoH, Sprite::Index::Outline, pal);
    fill_rect(idx, rgba, kTorsoX + 1, kTorsoY + 1, kTorsoW - 2, 3, Sprite::Index::Accent, pal);
    fill_rect(idx, rgba, kTorsoX + 1, kTorsoY + 4, kTorsoW - 2, kTorsoH - 5, Sprite::Index::Body, pal);

    const int arm_sway = (walk_frame == 0) ? 0 : 1;
    for (int side = 0; side < 2; ++side) {
        const int base_arm_x = (side == 0) ? kTorsoX - 2 : kTorsoX + kTorsoW - 1;
        const int arm_x = base_arm_x + ((side == 0) ? -arm_sway : arm_sway);
        fill_rect(idx, rgba, arm_x, kTorsoY + 1, 2, kTorsoH - 4, Sprite::Index::Body, pal);
    }

    if (walk_frame == 0) {
        outline_rect(idx, rgba, kTorsoX + 2, kLegY, 5, kLegH, Sprite::Index::Outline, pal);
        fill_rect(idx, rgba, kTorsoX + 3, kLegY + 1, 3, kLegH - 2, Sprite::Index::Body, pal);
        outline_rect(idx, rgba, kTorsoX + kTorsoW - 7, kLegY, 5, kLegH, Sprite::Index::Outline, pal);
        fill_rect(idx, rgba, kTorsoX + kTorsoW - 6, kLegY + 1, 3, kLegH - 2, Sprite::Index::Body, pal);
    } else {
        outline_rect(idx, rgba, kTorsoX + 4, kLegY, 5, kLegH, Sprite::Index::Outline, pal);
        fill_rect(idx, rgba, kTorsoX + 5, kLegY + 1, 3, kLegH - 2, Sprite::Index::Body, pal);
        outline_rect(idx, rgba, kTorsoX + kTorsoW - 9, kLegY, 5, kLegH, Sprite::Index::Outline, pal);
        fill_rect(idx, rgba, kTorsoX + kTorsoW - 8, kLegY + 1, 3, kLegH - 2, Sprite::Index::Body, pal);
    }
}

static void draw_jump(Sprite::Index* idx, std::uint8_t* rgba,
                      const art::FighterPalette& pal) {
    outline_rect(idx, rgba, kHeadX, kHeadY, kHeadW, kHeadH, Sprite::Index::Outline, pal);
    fill_rect(idx, rgba, kHeadX + 1, kHeadY + 1, kHeadW - 2, kHeadH / 2 - 1, Sprite::Index::Highlight, pal);
    fill_rect(idx, rgba, kHeadX + 1, kHeadY + kHeadH / 2, kHeadW - 2, kHeadH / 2 - 1, Sprite::Index::Accent, pal);

    outline_rect(idx, rgba, kTorsoX, kTorsoY, kTorsoW, kTorsoH, Sprite::Index::Outline, pal);
    fill_rect(idx, rgba, kTorsoX + 1, kTorsoY + 1, kTorsoW - 2, 3, Sprite::Index::Accent, pal);
    fill_rect(idx, rgba, kTorsoX + 1, kTorsoY + 4, kTorsoW - 2, kTorsoH - 5, Sprite::Index::Body, pal);

    for (int side = 0; side < 2; ++side) {
        const int arm_x = (side == 0) ? kTorsoX - 1 : kTorsoX + kTorsoW;
        fill_rect(idx, rgba, arm_x, kTorsoY + 2, 1, kTorsoH - 6, Sprite::Index::Body, pal);
    }

    const int tuck_x = kTorsoX + 6;
    const int tuck_w = kTorsoW - 12;
    const int tuck_y = kLegY + 2;
    const int tuck_h = kLegH - 4;
    outline_rect(idx, rgba, tuck_x, tuck_y, tuck_w, tuck_h, Sprite::Index::Outline, pal);
    fill_rect(idx, rgba, tuck_x + 1, tuck_y + 1, tuck_w - 2, tuck_h - 2, Sprite::Index::Body, pal);
}

static void draw_attack(Sprite::Index* idx, std::uint8_t* rgba,
                        const art::FighterPalette& pal) {
    outline_rect(idx, rgba, kHeadX, kHeadY, kHeadW, kHeadH, Sprite::Index::Outline, pal);
    fill_rect(idx, rgba, kHeadX + 1, kHeadY + 1, kHeadW - 2, kHeadH / 2 - 1, Sprite::Index::Highlight, pal);
    fill_rect(idx, rgba, kHeadX + 1, kHeadY + kHeadH / 2, kHeadW - 2, kHeadH / 2 - 1, Sprite::Index::Accent, pal);

    outline_rect(idx, rgba, kTorsoX, kTorsoY, kTorsoW, kTorsoH, Sprite::Index::Outline, pal);
    fill_rect(idx, rgba, kTorsoX + 1, kTorsoY + 1, kTorsoW - 2, 3, Sprite::Index::Accent, pal);
    fill_rect(idx, rgba, kTorsoX + 1, kTorsoY + 4, kTorsoW - 2, kTorsoH - 5, Sprite::Index::Body, pal);

    // Left arm hanging
    fill_rect(idx, rgba, kTorsoX - 2, kTorsoY + 1, 2, kTorsoH - 4, Sprite::Index::Body, pal);

    // Right arm extended forward (sprite-space right; renderer mirrors
    // when facing left). Arm + fist extend into the right margin so
    // they show up after the body region.
    const int arm_y = kTorsoY + 3;
    for (int x = kTorsoX + kTorsoW; x < kTorsoX + kTorsoW + 8; ++x) {
        put(idx, rgba, x, arm_y,     Sprite::Index::Body, pal);
        put(idx, rgba, x, arm_y + 1, Sprite::Index::Body, pal);
        put(idx, rgba, x, arm_y + 2, Sprite::Index::Body, pal);
    }
    const int fist_x = kTorsoX + kTorsoW + 8;
    const int fist_y = arm_y - 1;
    outline_rect(idx, rgba, fist_x, fist_y, 4, 4, Sprite::Index::Outline, pal);
    fill_rect(idx, rgba, fist_x + 1, fist_y + 1, 2, 2, Sprite::Index::Accent, pal);

    outline_rect(idx, rgba, kTorsoX + 2, kLegY, 5, kLegH, Sprite::Index::Outline, pal);
    fill_rect(idx, rgba, kTorsoX + 3, kLegY + 1, 3, kLegH - 2, Sprite::Index::Body, pal);
    outline_rect(idx, rgba, kTorsoX + kTorsoW - 7, kLegY, 5, kLegH, Sprite::Index::Outline, pal);
    fill_rect(idx, rgba, kTorsoX + kTorsoW - 6, kLegY + 1, 3, kLegH - 2, Sprite::Index::Body, pal);
}

static void draw_hitstun(Sprite::Index* idx, std::uint8_t* rgba,
                         const art::FighterPalette& pal) {
    outline_rect(idx, rgba, kHeadX, kHeadY - 1, kHeadW, kHeadH, Sprite::Index::Outline, pal);
    fill_rect(idx, rgba, kHeadX + 1, kHeadY,     kHeadW - 2, kHeadH / 2 - 1, Sprite::Index::Highlight, pal);
    fill_rect(idx, rgba, kHeadX + 1, kHeadY + kHeadH / 2 - 1, kHeadW - 2, kHeadH / 2, Sprite::Index::Accent, pal);

    outline_rect(idx, rgba, kTorsoX, kTorsoY, kTorsoW, kTorsoH, Sprite::Index::Outline, pal);
    fill_rect(idx, rgba, kTorsoX + 1, kTorsoY + 1, kTorsoW - 2, 3, Sprite::Index::Accent, pal);
    fill_rect(idx, rgba, kTorsoX + 1, kTorsoY + 4, kTorsoW - 2, kTorsoH - 5, Sprite::Index::Body, pal);

    for (int side = 0; side < 2; ++side) {
        const int arm_x = (side == 0) ? kTorsoX - 3 : kTorsoX + kTorsoW - 1;
        fill_rect(idx, rgba, arm_x, kTorsoY + 1, 3, kTorsoH - 4, Sprite::Index::Body, pal);
    }

    outline_rect(idx, rgba, kTorsoX + 1, kLegY, 4, kLegH, Sprite::Index::Outline, pal);
    fill_rect(idx, rgba, kTorsoX + 2, kLegY + 1, 2, kLegH - 2, Sprite::Index::Body, pal);
    outline_rect(idx, rgba, kTorsoX + kTorsoW - 5, kLegY, 4, kLegH, Sprite::Index::Outline, pal);
    fill_rect(idx, rgba, kTorsoX + kTorsoW - 4, kLegY + 1, 2, kLegH - 2, Sprite::Index::Body, pal);
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

Sprite::Sprite(const art::FighterPalette& palette) {
    // M6-A single-frame Sprite: one Idle pose. The M6-A test asserts
    // frame_count() == 1 here.
    frames_.push_back(Frame{});
    current_frame_ = 0;
    draw_idle(frames_[0].index_map.data(), frames_[0].pixels.data(), palette);
    frames_[0].opaque_count = count_opaque(frames_[0].index_map.data());
}

Sprite Sprite::build_animation_strip(const art::FighterPalette& palette) {
    Sprite s(palette);
    // The constructor above already built frame 0 (Idle). Append the
    // remaining 5 frames in the order declared by kFrameOffsets.
    s.frames_.push_back(Frame{});
    draw_walk(s.frames_[1].index_map.data(), s.frames_[1].pixels.data(), palette, 0);
    s.frames_[1].opaque_count = count_opaque(s.frames_[1].index_map.data());

    s.frames_.push_back(Frame{});
    draw_walk(s.frames_[2].index_map.data(), s.frames_[2].pixels.data(), palette, 1);
    s.frames_[2].opaque_count = count_opaque(s.frames_[2].index_map.data());

    s.frames_.push_back(Frame{});
    draw_jump(s.frames_[3].index_map.data(), s.frames_[3].pixels.data(), palette);
    s.frames_[3].opaque_count = count_opaque(s.frames_[3].index_map.data());

    s.frames_.push_back(Frame{});
    draw_attack(s.frames_[4].index_map.data(), s.frames_[4].pixels.data(), palette);
    s.frames_[4].opaque_count = count_opaque(s.frames_[4].index_map.data());

    s.frames_.push_back(Frame{});
    draw_hitstun(s.frames_[5].index_map.data(), s.frames_[5].pixels.data(), palette);
    s.frames_[5].opaque_count = count_opaque(s.frames_[5].index_map.data());

    return s;
}

void Sprite::set_pixel(int frame_index, int x, int y, Index idx,
                       const art::FighterPalette& pal) {
    if (frame_index < 0 || frame_index >= static_cast<int>(frames_.size())) return;
    put(frames_[static_cast<std::size_t>(frame_index)].index_map.data(),
        frames_[static_cast<std::size_t>(frame_index)].pixels.data(),
        x, y, idx, pal);
}

} // namespace ember
