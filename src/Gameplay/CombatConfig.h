#pragma once

// -----------------------------------------------------------------------------
// Combat tuning constants.
//
// Per the design brief (\u00a713 Damage / Knockback Model), the early prototype
// uses a deliberately simple model:
//
//     knockback = base_knockback * (1 + damage_percent / kKnockbackDamageDivisor)
//
// All three knobs live here so the formula can be retuned in one place. The
// system is designed so the formula itself can be swapped for a Smash-style
// polynomial later without changing the call sites in Match::resolve_hits.
// -----------------------------------------------------------------------------

namespace ember::combat {

// 100 means each +1% damage is +1% extra knockback. This is the spec's
// starting value; a more interesting feel may use 80 or 120.
inline constexpr int kKnockbackDamageDivisor = 100;

// Safety cap so a runaway damage% can't produce a single-frame launch that's
// fast enough to tunnel through a platform before collision runs. The actual
// magnitude after the cap is still applied via the formula in Match.
inline constexpr int kMaxDamagePercent = 999;

// Minimum hitstun frames so even the weakest poke gives the target at least
// a brief moment of visible reaction. Without this, a 0-hitstun attack would
// not actually be useful for combo extension in a future M3+ milestone.
inline constexpr int kMinHitstunFrames = 4;

} // namespace ember::combat
