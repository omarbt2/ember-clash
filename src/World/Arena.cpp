#include "World/Arena.h"

#include "Core/Config.h"
#include "Core/Logger.h"
#include "Gameplay/ArtConfig.h"
#include "Graphics/Renderer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace ember {

Arena::Arena() {
    reset_default();
}

void Arena::reset(Id id) {
    const float W = static_cast<float>(config::kRenderWidth);
    const float H = static_cast<float>(config::kRenderHeight);
    const float floor_top = H - 80.0f;
    const float thick = 24.0f;
    id_ = id;
    count_ = 0;

    auto push = [&](float x, float y, float w, float h,
                    bool one_way = false, bool ledge_grabbable = false) {
        if (count_ < kMaxPlatforms) {
            data_[static_cast<std::size_t>(count_++)] =
                Platform{Aabb{x, y, w, h}, one_way, ledge_grabbable};
        }
    };

    switch (id_) {
        case Id::TrainingRing:
            // Familiar flat floor with two elevated side platforms.
            push(0.0f, floor_top, W, H - floor_top, false, true);
            push(280.0f, floor_top - 160.0f, 240.0f, thick, true);
            push(W - 520.0f, floor_top - 160.0f, 240.0f, thick, true);
            spawn_x_[0] = 220.0f;
            spawn_x_[1] = W - 284.0f;
            break;
        case Id::EmberFoundry:
            // A narrow main deck with a central furnace bridge and offset
            // side shelves; the outer gaps create meaningful edge pressure.
            push(150.0f, floor_top, W - 300.0f, H - floor_top, false, true);
            push(450.0f, floor_top - 190.0f, 380.0f, thick, true);
            push(255.0f, floor_top - 95.0f, 190.0f, thick, true);
            push(W - 445.0f, floor_top - 95.0f, 190.0f, thick, true);
            spawn_x_[0] = 295.0f;
            spawn_x_[1] = W - 343.0f;
            break;
        case Id::SkyTemple:
            // Three stepped platforms over a raised central sanctuary.
            push(130.0f, floor_top, W - 260.0f, H - floor_top, false, true);
            push(235.0f, floor_top - 118.0f, 205.0f, thick, true);
            push(520.0f, floor_top - 215.0f, 240.0f, thick, true);
            push(W - 440.0f, floor_top - 118.0f, 205.0f, thick, true);
            spawn_x_[0] = 275.0f;
            spawn_x_[1] = W - 323.0f;
            break;
        default:
            id_ = Id::TrainingRing;
            push(0.0f, floor_top, W, H - floor_top, false, true);
            push(280.0f, floor_top - 160.0f, 240.0f, thick, true);
            push(W - 520.0f, floor_top - 160.0f, 240.0f, thick, true);
            spawn_x_[0] = 220.0f;
            spawn_x_[1] = W - 284.0f;
            break;
    }

    blast_zone_ = BlastZone{
        /*left=*/-170.0f,
        /*right=*/W + 170.0f,
        /*top=*/-180.0f,
        /*bottom=*/H + 200.0f,
    };
    spawn_y_ = floor_top;
}

float Arena::spawn_x(int player_index) const {
    return spawn_x_[(player_index == 0) ? 0 : 1];
}

bool Arena::is_out_of_bounds(const Aabb& body) const {
    return body.right() < blast_zone_.left
        || body.x > blast_zone_.right
        || body.bottom() < blast_zone_.top
        || body.y > blast_zone_.bottom;
}

bool Arena::resolve_collision(Aabb& body, Vec2& velocity,
                              bool drop_through_one_way) const {
    bool grounded = false;

    for (const Platform& p : platforms()) {
        if (!intersects(body, p.aabb)) {
            continue;
        }

        if (p.one_way) {
            if (drop_through_one_way) continue;
            // A one-way platform only catches a descending body whose feet
            // were above its top at the start of this substep. This also
            // prevents a fighter falling through from being pushed back up
            // on the following tick.
            const double previous_bottom = static_cast<double>(body.bottom()) - velocity.y;
            if (velocity.y <= 0.0 || previous_bottom > p.aabb.y + 0.001) {
                continue;
            }
            body.y = p.aabb.y - body.h;
            velocity.y = 0.0;
            grounded = true;
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


bool Arena::move_and_resolve(Aabb& body, Vec2& velocity,
                             bool drop_through_one_way) const {
    constexpr double kMaxSubstepDistance = 16.0;
    const double distance = std::hypot(velocity.x, velocity.y);
    if (!std::isfinite(distance)) {
        // Invalid simulation velocity must not poison positions or turn the
        // substep count conversion into undefined behaviour.
        velocity = Vec2{0.0, 0.0};
        return resolve_collision(body, velocity);
    }

    const double required_steps = std::max(
        1.0, std::ceil(distance / kMaxSubstepDistance));
    if (required_steps > static_cast<double>(std::numeric_limits<int>::max())) {
        velocity = Vec2{0.0, 0.0};
        return resolve_collision(body, velocity);
    }
    const int steps = static_cast<int>(required_steps);
    double step_x = velocity.x / static_cast<double>(steps);
    double step_y = velocity.y / static_cast<double>(steps);
    bool grounded = false;

    for (int i = 0; i < steps; ++i) {
        body.x += static_cast<float>(step_x);
        body.y += static_cast<float>(step_y);

        Vec2 step_velocity{step_x, step_y};
        grounded = resolve_collision(body, step_velocity, drop_through_one_way) || grounded;

        // Once an axis collides, stop advancing that component during the
        // remaining substeps; the unobstructed axis may still move.
        if (step_x != 0.0 && step_velocity.x == 0.0) {
            velocity.x = 0.0;
            step_x = 0.0;
        }
        if (step_y != 0.0 && step_velocity.y == 0.0) {
            velocity.y = 0.0;
            step_y = 0.0;
        }
    }
    return grounded;
}


LedgeGrab Arena::find_ledge_grab(const Aabb& body, const Vec2& velocity,
                                int facing) const {
    constexpr float kReach = 24.0f;
    constexpr float kHangOffsetBelowTop = 18.0f;
    LedgeGrab best{};
    float best_gap = std::numeric_limits<float>::max();

    if (velocity.y <= 0.0 || (facing != -1 && facing != 1)) return best;

    for (const Platform& platform : platforms()) {
        if (!platform.ledge_grabbable) continue;
        const Aabb& p = platform.aabb;
        // Ledge catching is only for an airborne body whose lower half has
        // passed the platform top. Bodies already above the surface should
        // go through the normal landing resolver instead.
        if (body.bottom() <= p.y + 2.0f
            || body.y > p.y + body.h * 0.45f) {
            continue;
        }

        const float left_gap = p.x - body.right();
        if (facing > 0 && left_gap >= -1.0f
            && left_gap <= kReach && left_gap < best_gap) {
            best = LedgeGrab{
                true,
                p.x - body.w + 6.0f,
                p.y - body.h + kHangOffsetBelowTop,
                p.x + 6.0f,
                p.y,
                1,
            };
            best_gap = left_gap;
        }

        const float right_gap = body.x - p.right();
        if (facing < 0 && right_gap >= -1.0f
            && right_gap <= kReach && right_gap < best_gap) {
            best = LedgeGrab{
                true,
                p.right() - 6.0f,
                p.y - body.h + kHangOffsetBelowTop,
                p.right() - body.w - 6.0f,
                p.y,
                -1,
            };
            best_gap = right_gap;
        }
    }
    return best;
}

bool Arena::supported_by_one_way(const Aabb& body) const {
    constexpr float kSupportTolerance = 1.0f;
    for (const Platform& p : platforms()) {
        if (!p.one_way) continue;
        const bool horizontal_overlap = body.right() > p.aabb.x
            && body.x < p.aabb.right();
        const bool feet_aligned = std::abs(body.bottom() - p.aabb.y)
            <= kSupportTolerance;
        if (horizontal_overlap && feet_aligned) return true;
    }
    return false;
}

void Arena::render(Renderer& renderer) const {
    // Procedural stage backdrop. The actual bottom blast plane sits below
    // the visible viewport, while the side/top margins are intentionally
    // kept outside the play field for clean KO detection.
    const float W = static_cast<float>(config::kRenderWidth);
    const float H = static_cast<float>(config::kRenderHeight);
    config::Color bg_top = art::kArenaBackgroundTop;
    config::Color bg_bottom = art::kArenaBackgroundBottom;
    config::Color platform_top = art::kArenaPlatformTop;
    config::Color platform_body = art::kArenaPlatformBody;
    config::Color platform_rim = art::kArenaPlatformRim;
    if (id_ == Id::EmberFoundry) {
        bg_top = {62, 26, 29, 255}; bg_bottom = {20, 10, 18, 255};
        platform_top = {235, 125, 60, 255}; platform_body = {110, 48, 40, 255};
        platform_rim = {255, 190, 95, 255};
    } else if (id_ == Id::SkyTemple) {
        bg_top = {26, 62, 82, 255}; bg_bottom = {8, 20, 42, 255};
        platform_top = {140, 205, 215, 255}; platform_body = {55, 105, 130, 255};
        platform_rim = {225, 235, 190, 255};
    }
    renderer.draw_vertical_gradient({0.0f, 0.0f, W, H}, bg_top, bg_bottom);
    for (const Platform& p : platforms()) {
        // Body: one solid color.
        renderer.draw_filled_rect(
            {p.aabb.x, p.aabb.y, p.aabb.w, p.aabb.h},
            platform_body);
        // Top rim: a thin highlight strip on the top edge of the
        // platform so it reads as a 3D surface and not a flat color.
        const float rim = std::min<float>(4.0f, p.aabb.h);
        renderer.draw_filled_rect(
            {p.aabb.x, p.aabb.y, p.aabb.w, rim},
            platform_top);
        // Outline: bright yellow on the main floor only, so the side
        // platforms stay quieter than the center stage.
        const bool is_main_floor = (p.aabb.w >= W - 1.0f);
        if (is_main_floor) {
            renderer.draw_rect_outline(
                {p.aabb.x, p.aabb.y, p.aabb.w, p.aabb.h},
                platform_rim);
        }
    }
}

} // namespace ember
