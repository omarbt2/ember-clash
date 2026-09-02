#pragma once

#include <cstdint>

namespace ember::config {

// -----------------------------------------------------------------------------
// Window
// -----------------------------------------------------------------------------
inline constexpr const char* kWindowTitle  = "Ember Clash";
inline constexpr int         kWindowWidth  = 1280;
inline constexpr int         kWindowHeight = 720;

// Logical render size used by the renderer. The window may be larger on HiDPI
// displays; we render at this resolution and let SDL scale to fit.
inline constexpr int kRenderWidth  = 1280;
inline constexpr int kRenderHeight = 720;

// -----------------------------------------------------------------------------
// Timing
// -----------------------------------------------------------------------------
// Target frames per second for the fixed-timestep simulation step.
inline constexpr int kTargetFps = 60;

// Cap delta time to avoid huge simulation jumps when the window is paused,
// dragged between monitors, or after long blocking operations.
inline constexpr double kMaxDeltaSeconds = 1.0 / 15.0;

// -----------------------------------------------------------------------------
// Placeholder palette (used by the M1 visual smoke-test)
// -----------------------------------------------------------------------------
struct Color {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;
};

inline constexpr Color kClearColor  { 18,  22,  32, 255 };
inline constexpr Color kArenaColor  { 70, 130, 180, 255 };
inline constexpr Color kFighterP1   { 220,  80,  70, 255 };
inline constexpr Color kFighterP2   {  70, 130, 220, 255 };
inline constexpr Color kAccentColor { 240, 200,  80, 255 };

} // namespace ember::config