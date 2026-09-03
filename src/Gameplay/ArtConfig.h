#pragma once

#include "Core/Config.h"

#include <cstdint>

// =============================================================================
// M6 — Art direction constants
// =============================================================================
// All visual identity (palette, silhouette sizes, sprite dimensions) lives
// here so future art changes happen in exactly one place. No PNGs, no BMPs,
// no asset files: the M6 fighter sprite is procedurally generated in code
// from these constants, which keeps the project dependency-free and makes
// the art testable from a headless harness.
//
// Palette rules:
//   * P1 = warm (orange / red)
//   * P2 = cool (blue / cyan)
//   * Both fighters share the same silhouette and same per-slot tint
//     intensities; only the base hue changes. This keeps the animation
//     system (M6-B) simple: one sprite sheet, one set of frame indices.
//   * The arena background + platform fill is intentionally low-contrast
//     so the fighters stay the visual focus.
//
namespace ember::art {

// Logical dimensions of one fighter sprite (in source pixels). 64x64 is big
// enough for a readable silhouette but small enough that we can have
// several frames in M6-B without bloating the texture.
inline constexpr int kFighterSpriteSize = 64;

// Each fighter body is 48x64 source pixels. The sprite includes a 8-px
// margin on each side so the silhouette can extend (arms, weapons) without
// touching the texture edge. The body box is what the physics/collision
// code treats as the fighter; the sprite is drawn centered on that body
// so visuals and physics stay in sync.
inline constexpr int kFighterBodyWidth  = 48;
inline constexpr int kFighterBodyHeight = 64;

// Logical pixel size: every source pixel becomes kRenderPixelSize logical
// render pixels. At 1280x720 and a 48x64 fighter, kRenderPixelSize=2 gives
// a 96x128 on-screen fighter, which reads well without dominating the
// arena.
inline constexpr int kRenderPixelSize = 2;

// -----------------------------------------------------------------------------
// Palette: per-fighter color tints.
// -----------------------------------------------------------------------------
// Each tint is a 3-stop ramp: dark (outline), mid (body), light (highlight).
// The procedural sprite generator reads these directly, so changing the
// palette changes every fighter in the game in one edit.
struct FighterPalette {
    config::Color outline;  // dark stop; used for the silhouette outline
    config::Color body;     // primary body fill
    config::Color accent;   // secondary body fill (e.g. sash, shoulder pad)
    config::Color highlight;// specular highlight on the top of the head/torso
};

inline constexpr FighterPalette kPaletteP1{
    /*outline   */ { 60,  18,  10, 255 },
    /*body      */ {220,  80,  60, 255 },
    /*accent    */ {245, 150,  50, 255 },
    /*highlight */ {255, 220, 150, 255 },
};

inline constexpr FighterPalette kPaletteP2{
    /*outline   */ { 10,  22,  60, 255 },
    /*body      */ { 70, 130, 220, 255 },
    /*accent    */ {110, 200, 245, 255 },
    /*highlight */ {200, 235, 255, 255 },
};

// -----------------------------------------------------------------------------
// Arena palette.
// -----------------------------------------------------------------------------
// Distinct from the fighter palette so fighters pop off the background.
inline constexpr config::Color kArenaBackgroundTop    {  18,  24,  44, 255 };
inline constexpr config::Color kArenaBackgroundBottom {   8,  12,  24, 255 };
inline constexpr config::Color kArenaPlatformTop     {  82, 150, 200, 255 };
inline constexpr config::Color kArenaPlatformBody    {  56, 110, 160, 255 };
inline constexpr config::Color kArenaPlatformRim     { 240, 220,  90, 255 };
inline constexpr config::Color kArenaDeathZoneTint   { 140,  30,  40, 255 };

// Hitspark / impact effect colour (M6-C will animate it).
inline constexpr config::Color kHitSparkInner        { 255, 255, 230, 255 };
inline constexpr config::Color kHitSparkOuter        { 250, 180,  60, 255 };

} // namespace ember::art
