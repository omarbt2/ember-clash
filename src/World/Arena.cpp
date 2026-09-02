#include "World/Arena.h"

#include "Core/Config.h"
#include "Core/Logger.h"
#include "Graphics/Renderer.h"

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
    const float side_margin = 80.0f;

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
    for (const Platform& p : platforms()) {
        renderer.draw_filled_rect(
            {p.aabb.x, p.aabb.y, p.aabb.w, p.aabb.h},
            config::kArenaColor);
        renderer.draw_rect_outline(
            {p.aabb.x, p.aabb.y, p.aabb.w, p.aabb.h},
            config::kAccentColor);
    }
}

} // namespace ember
