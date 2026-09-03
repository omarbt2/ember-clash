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
// Core palette. The placeholder shapes from M1 were promoted to the real
// M6-A sprite generator (see Sprite.cpp) and the M6-B multi-frame palette
// reads these constants. No more "M1 smoke-test" - all of these drive
// shipped art.
// -----------------------------------------------------------------------------
struct Color {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;
};

inline constexpr Color kClearColor       { 18,  22,  32, 255 };
inline constexpr Color kArenaColor       { 70, 130, 180, 255 };
inline constexpr Color kFighterP1        { 220,  80,  70, 255 };
inline constexpr Color kFighterP2        {  70, 130, 220, 255 };
inline constexpr Color kAccentColor      { 240, 200,  80, 255 };
// M5 HUD palette. Damage text + stock pips mirror each fighter's color so the
// player can read their state at a glance. The KO banner is a bright
// yellow-on-dark for high contrast over a busy arena.
inline constexpr Color kHudTextColor    { 245, 245, 250, 255 };
inline constexpr Color kHudStockColor   { 240, 220,  80, 255 };
inline constexpr Color kHudStockLostColor{  60,  60,  70, 255 };
inline constexpr Color kKoBannerColor   { 250, 230,  90, 255 };
inline constexpr Color kHitSparkColor   { 255, 255, 200, 255 };

// Color-blind HUD ink. Used in place of kHudTextColor when
// Settings::color_blind_palette is on. The colour is bright
// yellow (CUD-recommended) so the damage % stays distinct
// against any background the arena could draw. The KO banner
// keeps kKoBannerColor (already yellow); only the damage %
// ink is swapped.
inline constexpr Color kHudTextColorColorBlind{ 255, 220,  60, 255 };

} // namespace ember::config