#include "World/Arena.h"

#include "Core/Config.h"
#include "Core/Logger.h"
#include "Gameplay/ArtConfig.h"
#include "Graphics/Renderer.h"

#include <algorithm>

namespace ember {

Arena::Arena() {
    reset_default();
}

void Arena::reset_default() {
    // Layout matches the spec §10 ASCII diagram: one main floor across the
    // whole stage and two side platforms elevated above it. Tuned to the
    // logical render size so the same arena scales with the window.
    const float W  = static_cast<float>(config::kRenderWidth);
    const float H  = static_cast<float>(config::kRenderHeight);
    const float floor_top   = H - 80.0f;          // main platform top y
    const float side_top    = floor_top - 160.0f; // side platforms 160 px higher
    const float thick       = 24.0f;              // platform thickness
    const float side_w      = 240.0f;
    // Side platforms sit between the two spawn points so a fresh-spawn
    // jump (body directly above the spawn x) does not bonk the platform
    // ceiling. The previous 80-px margin placed the left platform at
    // x=80..320, which fully covered the P1 spawn body (x=220..268),
    // so a straight-up jump from spawn zeroed the rise velocity at the
    // platform's underside. 280 px leaves a clean spawn column and
    // still gives a 240-px-wide side platform to fight on.
    const float side_margin = 280.0f;

    count_ = 0;

    auto push = [&](float x, float y, float w, float h) {
        data_[count_++] = Platform{Aabb{x, y, w, h}};
    };

    // Main platform: full width.
    push(0.0f, floor_top, W, H - floor_top);
    // Left side platform.
    push(side_margin, side_top, side_w, thick);
    // Right side platform.
    push(W - side_margin - side_w, side_top, side_w, thick);

    // Death zone sits well below the floor; fighters that fall past it die.
    death_zone_y_ = H + 200.0f;

    // Spawn position: a little above the main platform so the fighter's feet
    // are flush with the floor on the first frame.
    spawn_y_   = floor_top;
    spawn_x_[0] = 220.0f;
    spawn_x_[1] = W - 220.0f - 64.0f;  // 64 = fighter width
}

float Arena::spawn_x(int player_index) const {
    return spawn_x_[(player_index == 0) ? 0 : 1];
}

bool Arena::resolve_collision(Aabb& body, Vec2& velocity) const {
    bool grounded = false;

    for (const Platform& p : platforms()) {
        if (!intersects(body, p.aabb)) {
            continue;
        }

        // How far we'd need to push the body on each side to clear the
        // platform. Positive values mean there's an actual overlap.
        const float push_left  = p.aabb.right()  - body.x;        // push body left
        const float push_right = (body.x + body.w) - p.aabb.x;    // push body right
        // push_up: vertical distance from body bottom to platform top.
        //   Positive = body has come down INTO the platform (landing case).
        //   Resolution: set body.y = p.aabb.y - body.h so feet rest on top.
        const float push_up    = (body.y + body.h) - p.aabb.y;
        // push_down: vertical distance from platform bottom to body top.
        //   Positive = body has come up INTO the platform (bonk case).
        //   Resolution: set body.y = p.aabb.bottom() so head rests under.
        const float push_down  = p.aabb.bottom() - body.y;

        // Resolve along the axis of smallest penetration. Ties go vertical
        // (Smash-style fighters prioritize landing/bonking over wall-sliding).
        const float min_h = std::min(push_left, push_right);
        const float min_v = std::min(push_up, push_down);

        if (min_h < min_v) {
            if (push_left < push_right) {
                body.x = p.aabb.right();
                if (velocity.x > 0.0) velocity.x = 0.0;
            } else {
                body.x = p.aabb.x - body.w;
                if (velocity.x < 0.0) velocity.x = 0.0;
            }
        } else {
            if (push_up < push_down) {
                // Body came from above -> land on the platform.
                body.y = p.aabb.y - body.h;
                if (velocity.y > 0.0) {
                    velocity.y = 0.0;
                    grounded = true;
                }
            } else {
                // Body came from below -> bonk the head.
                body.y = p.aabb.bottom();
                if (velocity.y < 0.0) velocity.y = 0.0;
            }
        }
    }

    return grounded;
}

void Arena::render(Renderer& renderer) const {
    // M6-A: vertical gradient background (top -> bottom) for soft depth.
    // The whole stage is one band, so we can pass a single rect spanning
    // the render width. The death-zone tint below the floor is the same
    // gradient, so a fighter that falls into it visually warns the player
    // they are out of bounds.
    const float W = static_cast<float>(config::kRenderWidth);
    const float H = static_cast<float>(config::kRenderHeight);
    renderer.draw_vertical_gradient(
        { 0.0f, 0.0f, W, H },
        art::kArenaBackgroundTop,
        art::kArenaBackgroundBottom);
    // Death-zone band: a thin red strip just below the floor to telegraph
    // where the kill plane is. M6 will animate the strip later; for now
    // a static band is enough to communicate the rule.
    if (death_zone_y_ < H + 32.0f) {
        renderer.draw_filled_rect(
            { 0.0f, death_zone_y_ - 6.0f, W, 12.0f },
            art::kArenaDeathZoneTint);
    }

    for (const Platform& p : platforms()) {
        // Body: one solid color.
        renderer.draw_filled_rect(
            {p.aabb.x, p.aabb.y, p.aabb.w, p.aabb.h},
            art::kArenaPlatformBody);
        // Top rim: a thin highlight strip on the top edge of the
        // platform so it reads as a 3D surface and not a flat color.
        const float rim = std::min<float>(4.0f, p.aabb.h);
        renderer.draw_filled_rect(
            {p.aabb.x, p.aabb.y, p.aabb.w, rim},
            art::kArenaPlatformTop);
        // Outline: bright yellow on the main floor only, so the side
        // platforms stay quieter than the center stage.
        const bool is_main_floor = (p.aabb.w >= W - 1.0f);
        if (is_main_floor) {
            renderer.draw_rect_outline(
                {p.aabb.x, p.aabb.y, p.aabb.w, p.aabb.h},
                art::kArenaPlatformRim);
        }
    }
}

} // namespace ember
