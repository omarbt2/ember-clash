#pragma once

// -----------------------------------------------------------------------------
// Small 2D geometry primitives shared by the simulation and the renderer.
//
// Everything is in render-space pixels with the SDL convention: x grows right,
// y grows down. The header is intentionally inline-only because the types are
// tiny and the helpers are trivial; there is no associated .cpp file.
// -----------------------------------------------------------------------------

namespace ember {

struct Vec2 {
    double x{0.0};
    double y{0.0};
};

struct Aabb {
    float x{0.0f};
    float y{0.0f};
    float w{0.0f};
    float h{0.0f};

    constexpr float left()   const { return x; }
    constexpr float right()  const { return x + w; }
    constexpr float top()    const { return y; }
    constexpr float bottom() const { return y + h; }
};

inline constexpr bool intersects(const Aabb& a, const Aabb& b) {
    return a.left() < b.right()  && a.right()  > b.left()
        && a.top()  < b.bottom() && a.bottom() > b.top();
}

// Center of an AABB in world space.
inline constexpr Vec2 center(const Aabb& a) {
    return { a.x + a.w * 0.5, a.y + a.h * 0.5 };
}

} // namespace ember
