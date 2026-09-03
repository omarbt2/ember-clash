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

// ------------------------------------------------------------------
// Color-blind alternate palettes.
//
// These are used when the user enables
// Settings::color_blind_palette. The shapes are kept identical (the
// silhouette is the same), but the hue choices follow the standard
// Color Universal Design recommendations: yellow vs blue (the
// two colors that stay distinct under every common form of
// color-vision deficiency, including protanopia, deuteranopia,
// and tritanopia). The accent stops are intentionally more
// saturated to compensate for the lower contrast.
//
// P1 stays warm (it remains the "primary" fighter); the body
// and highlight are pushed toward bright yellow. P2 stays cool
// but the body is pushed toward bright blue with a light cyan
// highlight.
//
// These constants are constexpr so the inline accessor
// palette_for(player_index, color_blind_on) is a compile-time
// branch.
// ------------------------------------------------------------------
inline constexpr FighterPalette kPaletteP1ColorBlind{
    /*outline   */ { 40,  20,   0, 255 },
    /*body      */ {230, 175,  20, 255 },
    /*accent    */ {255, 220,  60, 255 },
    /*highlight */ {255, 245, 200, 255 },
};

inline constexpr FighterPalette kPaletteP2ColorBlind{
    /*outline   */ {  0,  10,  35, 255 },
    /*body      */ { 30, 110, 220, 255 },
    /*accent    */ { 80, 180, 245, 255 },
    /*highlight */ {200, 240, 255, 255 },
};

// Resolve the palette for a given fighter and accessibility
// setting. Compile-time-friendly: the color_blind_on bool can
// be elided when false.
constexpr const FighterPalette& palette_for(int player_index, bool color_blind_on) {
    if (color_blind_on) {
        return player_index == 0 ? kPaletteP1ColorBlind
                                 : kPaletteP2ColorBlind;
    }
    return player_index == 0 ? kPaletteP1 : kPaletteP2;
}

// Runtime color-blind flag. Stored as a function-local static
// (so the symbol does not leak) and read at Fighter construction
// time. The flag is consulted when Sprite::build_animation_strip()
// is called inside Fighter::Fighter, which happens on every
// Match::reset() (initial construction + rematch). Toggling the
// flag between matches is therefore the supported workflow; the
// next rematch picks up the new palette.
//
// We keep the toggle function-local (not a global) so the
// declaration stays in the same header that defines the
// palettes, and so the address is well-defined without an
// out-of-line definition.
inline bool& color_blind_flag() {
    static bool enabled = false;
    return enabled;
}

// Returns the current state of the runtime color-blind flag.
inline bool color_blind_enabled() { return color_blind_flag(); }

// Sets the runtime color-blind flag.
inline void set_color_blind(bool on) { color_blind_flag() = on; }

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
