#pragma once

#include "Core/Config.h"
#include "Gameplay/CombatConfig.h"
#include "Gameplay/Match.h"

#include <array>
#include <string>

namespace ember::ui {

// Hud draws the M5 match-flow overlay: stock pips + damage % per fighter,
// the KO banner, and the rematch prompt. It is a pure renderer - it never
// mutates Match, so the test harness can call its functions in isolation
// without setting up a window.
class Hud {
public:
    // Draws a single fighter's per-frame HUD: stock pips + damage % in the
    // top corner indicated by `side` (0 = top-left for P1, 1 = top-right for
    // P2). The fighter's player color is used for the stock pips; damage
    // uses the HUD text color.
    static void draw_fighter_card(Renderer& renderer,
                                  const Fighter& fighter,
                                  int side);

    // Renders the per-fighter damage % floating above the fighter's head.
    // Skips dead fighters (Match::render() also skips them).
    static void draw_damage_floats(Renderer& renderer, const Fighter& fighter);

    // Draws the active hit-spark (a small + sign) at the supplied world
    // position. Used by Match::render().
    static void draw_hit_spark(Renderer& renderer, float x, float y, int age);

    // Draws the "GAME!" banner if the match is over and the initial banner
    // timer hasn't elapsed yet. Banner text shrinks/changes after the timer
    // to show the rematch prompt.
    static void draw_ko_banner(Renderer& renderer, const Match& match,
                               const Fighter& p1, const Fighter& p2);

    // P15: draws the READY/FIGHT intro banner while
    // Match::is_intro_active() is true. After the banner clears
    // (intro_banner_state_ == 2) this is a no-op so it can be
    // called unconditionally from Match::render(). The banner
    // uses the same KO-band layout so the visual treatment
    // matches the post-match banner (dark band + yellow text).
    static void draw_intro_banner(Renderer& renderer, const Match& match);
};

// -----------------------------------------------------------------------------
// Small helpers used by the HUD and by tests. Kept in this header so the
// scratch tests in tests/ can reuse the digit-to-string routine without
// pulling in the rest of the Hud class.
// -----------------------------------------------------------------------------

// Writes a non-negative integer into `out` as ASCII digits followed by the
// percent sign (e.g. 47 -> "47%"). Wide enough for combat::kMaxDamagePercent
// (3 digits) + '%'. The character-by-character walk is the simplest
// option; the alternative `std::to_string(percent) + "%"` is one line
// longer once the clamp is included and yields the same output.
inline std::string format_damage(int percent) {
    if (percent < 0) percent = 0;
    if (percent > combat::kMaxDamagePercent) percent = combat::kMaxDamagePercent;
    std::string out;
    const int hundreds = percent / 100;
    const int tens     = (percent / 10) % 10;
    const int ones     = percent % 10;
    if (hundreds > 0) {
        out.push_back(static_cast<char>('0' + hundreds));
        out.push_back(static_cast<char>('0' + tens));
    } else if (tens > 0) {
        out.push_back(static_cast<char>('0' + tens));
    }
    out.push_back(static_cast<char>('0' + ones));
    out.push_back('%');
    return out;
}

} // namespace ember::ui
