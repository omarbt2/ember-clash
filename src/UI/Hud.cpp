#include "UI/Hud.h"

#include "Core/Config.h"
#include "Core/Math.h"
#include "Gameplay/AiController.h"
#include "Gameplay/ArtConfig.h"
#include "Gameplay/Fighter.h"
#include "Gameplay/Match.h"
#include "Graphics/Renderer.h"
#include "UI/BitmapFont.h"

#include <cstdio>

namespace ember::ui {

namespace {

// All coordinates below are in render-space pixels (see config::kRenderWidth
// / kRenderHeight). The HUD layout is fixed and is not HiDPI-scaled - the
// renderer's letterboxing handles window vs logical size.

constexpr int kMargin         = 16;
constexpr int kCardHeight     = 56;
constexpr int kCardWidth      = 220;
constexpr int kStockPipW      = 22;
constexpr int kStockPipH      = 10;
constexpr int kStockPipGap    = 4;
constexpr int kDamageY        = 8;
constexpr int kDamageFontScale = 2;  // 2x means a "47%" string is ~ 8*15 = 120 px
constexpr int kKoBannerY      = 160;

config::Color player_color(int player_index) {
    return (player_index == 0) ? config::kFighterP1 : config::kFighterP2;
}

// Draw a single character at (x, y) at the given integer scale. Higher
// scales are used for the per-fighter damage text and the KO banner.
void draw_glyph_scaled(Renderer& renderer, int x, int y, char c,
                       config::Color color, int scale) {
    const auto& g = detail::glyph_for(c);
    const int cell = kCellPx * scale;
    for (int row = 0; row < 6; ++row) {
        const std::uint8_t bits = g.rows[row];
        for (int col = 0; col < 4; ++col) {
            if ((bits >> (3 - col)) & 1) {
                renderer.draw_filled_rect(
                    { static_cast<float>(x + col * cell),
                      static_cast<float>(y + row * cell),
                      static_cast<float>(cell),
                      static_cast<float>(cell) },
                    color);
            }
        }
    }
}

// Draw a string at (x, y) at the given scale.
void draw_text_scaled(Renderer& renderer, int x, int y, const char* s,
                      config::Color color, int scale) {
    if (!s) return;
    int cx = x;
    for (; *s; ++s) {
        draw_glyph_scaled(renderer, cx, y, *s, color, scale);
        cx += kAdvance * scale;
    }
}

} // namespace

void Hud::draw_fighter_card(Renderer& renderer, const Fighter& fighter, int side) {
    const config::Color accent = player_color(fighter.player_index());
    const int card_x = (side == 0) ? kMargin
                                   : config::kRenderWidth - kMargin - kCardWidth;
    const int card_y = kMargin;

    // Stock pips: small filled rectangles, one per remaining stock. Lost
    // stocks are shown as dim outlines so the count is readable at a glance.
    const int pips_total = combat::kStartingStocks;
    const int pips_alive = fighter.stocks();
    for (int i = 0; i < pips_total; ++i) {
        const int pip_x = (side == 0)
            ? card_x + i * (kStockPipW + kStockPipGap)
            : card_x + kCardWidth - (i + 1) * (kStockPipW + kStockPipGap);
        const int pip_y = card_y;
        if (i < pips_alive) {
            renderer.draw_filled_rect(
                { static_cast<float>(pip_x),
                  static_cast<float>(pip_y),
                  static_cast<float>(kStockPipW),
                  static_cast<float>(kStockPipH) },
                accent);
        } else {
            renderer.draw_rect_outline(
                { static_cast<float>(pip_x),
                  static_cast<float>(pip_y),
                  static_cast<float>(kStockPipW),
                  static_cast<float>(kStockPipH) },
                config::kHudStockLostColor);
        }
    }

    // Damage % rendered as bitmap-font text. Left side: left-aligned; right
    // side: right-aligned. Text is 2x scale for readability at 1280x720.
    // The ink color picks the color-blind palette when that setting is
    // active, so players with reduced color-vision get a high-contrast
    // yellow readout instead of the default near-white.
    const std::string dmg = format_damage(fighter.damage_percent());
    const int dmg_w = static_cast<int>(dmg.size()) * kAdvance * kDamageFontScale
                    - kCellPx * kDamageFontScale;
    const int dmg_x = (side == 0) ? card_x
                                   : card_x + kCardWidth - dmg_w;
    const int dmg_y = card_y + kStockPipH + kDamageY;
    const config::Color dmg_color = art::color_blind_enabled()
        ? config::kHudTextColorColorBlind
        : config::kHudTextColor;
    draw_text_scaled(renderer, dmg_x, dmg_y, dmg.c_str(),
                     dmg_color, kDamageFontScale);
}

void Hud::draw_damage_floats(Renderer& renderer, const Fighter& fighter) {
    if (fighter.is_dead()) return;
    const std::string dmg = format_damage(fighter.damage_percent());
    const int w = static_cast<int>(dmg.size()) * kAdvance - kCellPx;
    const int x = static_cast<int>(fighter.body().x + fighter.body().w * 0.5f
                                   - w * 0.5f);
    const int y = static_cast<int>(fighter.body().y - kGlyphH - 4);
    // Same color-blind swap as draw_fighter_card so the floating
    // damage readout matches the card-strip damage.
    const config::Color dmg_color = art::color_blind_enabled()
        ? config::kHudTextColorColorBlind
        : config::kHudTextColor;
    draw_text(renderer, x, y, dmg.c_str(), dmg_color);
}

void Hud::draw_hit_spark(Renderer& renderer, float x, float y, int age) {
    // A simple + sign whose arms shrink as `age` advances. age == 0 is the
    // first frame (full size); age == combat::kHitSparkFrames is the last.
    const int total = combat::kHitSparkFrames;
    const float t = static_cast<float>(total - age) / static_cast<float>(total);
    const float arm_long  = 14.0f * t + 4.0f;
    const float arm_short = 3.0f;
    const config::Color color = config::kHitSparkColor;
    renderer.draw_filled_rect(
        { x - arm_long, y - arm_short, arm_long * 2.0f, arm_short * 2.0f },
        color);
    renderer.draw_filled_rect(
        { x - arm_short, y - arm_long, arm_short * 2.0f, arm_long * 2.0f },
        color);
}

void Hud::draw_ko_banner(Renderer& renderer, const Match& match,
                         const Fighter& p1, const Fighter& p2) {
    (void)p2;  // M5 1v1 only needs p1; symmetric signature is future-proof.
    if (match.phase() != Match::Phase::MatchOver) return;

    constexpr int band_h = 96;
    renderer.draw_filled_rect(
        { 0.0f, static_cast<float>(kKoBannerY),
          static_cast<float>(config::kRenderWidth),
          static_cast<float>(band_h) },
        { 12, 12, 18, 255 });

    const bool show_initial = match.ko_banner_timer_seconds()
                            < combat::kKoBannerInitialSeconds;

    // Static buffer so the pointer handed to draw_text_scaled stays valid
    // for the duration of the function. "P1 WINS - PRESS R" + null is 22
    // bytes; 48 is plenty.
    static char buf[48];
    const char* text;
    if (show_initial) {
        text = "GAME!";
    } else {
        const int winner = (p1.stocks() > 0) ? 0 : 1;
        std::snprintf(buf, sizeof(buf), "P%d WINS - PRESS R", winner + 1);
        text = buf;
    }
    const std::size_t n = std::char_traits<char>::length(text);
    const int w = static_cast<int>(n) * kAdvance * 2 - kCellPx * 2;
    const int x = (config::kRenderWidth - w) / 2;
    const int y = kKoBannerY + (band_h - kGlyphH * 2) / 2;
    draw_text_scaled(renderer, x, y, text, config::kKoBannerColor, 2);
}

} // namespace ember::ui
