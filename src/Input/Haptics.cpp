#include "Input/Haptics.h"

#include "Core/Logger.h"

namespace ember {

namespace {

// HapticsConfig: all tuning lives here. No magic numbers in
// the call sites. The values are the M14 starting point - the
// team will tweak them based on playtesting, but doing so does
// not require any code change outside this file.
struct HapticsConfig {
    // Strong hit: a quick, low-intensity tap. The high-freq
    // motor is used so the cue is felt as a sharp "click" in
    // the player's hands rather than a deep "thump" that
    // could be confused with a KO.
    std::uint16_t strong_hit_low  = 0;
    std::uint16_t strong_hit_high = 0xC000;        // ~75%
    int           strong_hit_ms   = 90;            // < 1/10 s

    // KO: a long, full-strength thump. The point of the KO
    // rumble is to celebrate, not to convey information, so
    // we go heavy.
    std::uint16_t ko_low  = 0xC000;                // ~75%
    std::uint16_t ko_high = 0xC000;                // ~75%
    int           ko_ms   = 350;                   // ~1/3 s

    // Menu confirm: a tiny, low-intensity tick. The UI
    // already has audio + visual cues; rumble is purely
    // supportive and must not feel like a hit.
    std::uint16_t menu_confirm_low  = 0;
    std::uint16_t menu_confirm_high = 0x3000;     // ~18%
    int           menu_confirm_ms   = 40;

    // Menu move: even softer. Haptics on every menu move
    // would be obnoxious; this is intentionally small enough
    // to be barely felt, just a texture cue.
    std::uint16_t menu_move_low  = 0;
    std::uint16_t menu_move_high = 0x1800;         // ~9%
    int           menu_move_ms   = 20;
};

const HapticsConfig& config() {
    static const HapticsConfig kCfg;
    return kCfg;
}

} // namespace

Haptics::Haptics() = default;

Haptics::~Haptics() {
    // Make sure the controller is not still vibrating when
    // we shut down. The destructor can run while the
    // gamepad layer is still alive (App destroys Haptics
    // first, then Gamepad), so this is the right place to
    // issue a final stop.
    stop_all();
}

void Haptics::play_strong_hit()  { start(Effect::kStrongHit); }
void Haptics::play_ko()          { start(Effect::kKo); }
void Haptics::play_menu_confirm(){ start(Effect::kMenuConfirm); }
void Haptics::play_menu_move()   { start(Effect::kMenuMove); }

void Haptics::start(Effect e) {
    ++effects_played_total_;
    if (!enabled_ || !gamepad_) return;

    // Mapping table: low/high/duration per effect.
    std::uint16_t low = 0, high = 0;
    int ms = 0;
    switch (e) {
        case Effect::kStrongHit:
            low  = config().strong_hit_low;
            high = config().strong_hit_high;
            ms   = config().strong_hit_ms;
            break;
        case Effect::kKo:
            low  = config().ko_low;
            high = config().ko_high;
            ms   = config().ko_ms;
            break;
        case Effect::kMenuConfirm:
            low  = config().menu_confirm_low;
            high = config().menu_confirm_high;
            ms   = config().menu_confirm_ms;
            break;
        case Effect::kMenuMove:
            low  = config().menu_move_low;
            high = config().menu_move_high;
            ms   = config().menu_move_ms;
            break;
        case Effect::kNone:
            return;
    }

    // Convert ms to 60Hz ticks. We round up so a 90ms rumble
    // still gets a tick (90 / 16.67 = 5.4, round up to 6).
    // The "ticks" approach keeps the test harness
    // deterministic: it can drive update() a known number of
    // times and observe the rumble stopping.
    int ticks = (ms + 16) / 17;
    if (ticks <= 0) ticks = 1;

    active_         = e;
    ticks_remaining_ = ticks;

    // Route to Gamepad. Gamepad::rumble() handles the "no
    // gamepad" and "no rumble motors" cases itself.
    gamepad_->rumble(low, high, static_cast<std::uint32_t>(ms));
}

void Haptics::update() {
    if (active_ == Effect::kNone) return;
    if (ticks_remaining_ > 0) {
        --ticks_remaining_;
    }
    if (ticks_remaining_ == 0) {
        if (gamepad_) gamepad_->stop_rumble();
        active_ = Effect::kNone;
    }
}

void Haptics::stop_all() {
    if (active_ != Effect::kNone) {
        if (gamepad_) gamepad_->stop_rumble();
    }
    active_          = Effect::kNone;
    ticks_remaining_ = 0;
}

} // namespace ember
