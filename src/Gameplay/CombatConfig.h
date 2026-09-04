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

// -----------------------------------------------------------------------------
// M5: stocks, respawn, and match flow.
//
// All match-flow tuning lives here so designers can change the rules without
// hunting through the codebase. The constants below are in fixed-step units
// (frames) or seconds depending on what they gate; the per-frame ticker in
// Match::update converts seconds to frames using config::kTargetFps.
// -----------------------------------------------------------------------------

// Number of lives each fighter starts a match with. 3 is the Smash standard.
inline constexpr int kStartingStocks = 3;

// Time a freshly-spawned fighter is invulnerable (in seconds). Prevents the
// "spawn trap" of an opponent camping the spawn platform and comboing the
// respawner before they can act.
inline constexpr double kRespawnInvulnSeconds = 1.5;

// How long the respawn animation blocks for before the fighter reappears
// (in seconds). This is the *invisible* respawn time; the fighter is off-
// stage during this window, so other systems can show effects over the empty
// platform if they want.
inline constexpr double kRespawnDelaySeconds = 0.5;

// How long the "GAME!" banner plays before the rematch prompt takes over
// (in seconds). Smash uses about 2.5s; we go a touch shorter to keep the
// rematch flow snappy on a 60Hz test rig.
inline constexpr double kKoBannerInitialSeconds = 1.5;

// Total number of hit-sparks the match can have alive at once. Three slots is
// plenty for two-fighter gameplay; if a third slot is needed we just don't
// spawn the new spark.
inline constexpr int kMaxHitSparks = 4;

// Number of frames a hit-spark is visible for. ~6 frames is enough to register
// without obscuring follow-up hits.
inline constexpr int kHitSparkFrames = 6;

// -----------------------------------------------------------------------------
// M6-C: combat feel tuning.
//
// These are the "punchy" feel constants. They are deliberately separated from
// the structural constants above so designers can change feel without
// touching rules.
// -----------------------------------------------------------------------------

// Number of frames the simulation pauses (motion + AI frozen, but the hit-
// spark still ages) when a hit connects. The classic Smash feel is 2-4
// freeze frames; 3 is a safe starting point for the first playable slice.
inline constexpr int kHitStopFrames = 3;

// -----------------------------------------------------------------------------
// P15: match intro sequence.
//
// A fresh match does not start with both fighters already in combat.
// Instead the match runs a short "READY..." -> "FIGHT!" banner
// (kIntroBannerReadyFrames + kIntroBannerFightFrames total frames)
// during which physics, AI, and hit resolution are frozen. This is
// the standard platform-fighter flow and is essential for playability:
// without it the match feels like the AI is already attacking
// the moment the player presses PLAY.
//
// Tuning rationale:
//   * 60 + 60 = 2.0 seconds total. Long enough to read the spawn
//     positions and recover from the menu click, short enough that
//     the player is not waiting. Smash uses about 1.7s (READY 1.0s
//     + FIGHT 0.7s); we go slightly longer to feel deliberate.
// -----------------------------------------------------------------------------
inline constexpr int kIntroBannerReadyFrames = 60;
inline constexpr int kIntroBannerFightFrames = 60;

// After the FIGHT! banner ends, the AI gets a kAiStartupGraceTicks
// window where it does NOT commit to an attack even if the human
// runs into range. This is the second half of the "give the human
// a chance to orient" fix - the banner alone is not enough because
// the human's first reaction is to move, and the AI would otherwise
// punish that movement on the first frame. 90 ticks = 1.5s of
// pure "stand and face the player" AI behavior.
inline constexpr int kAiStartupGraceTicks = 90;

// Minimum time (ticks) between two committed attacks by the AI.
// The AI's attack_frequency is still the per-tick probability of
// committing once in range, but this cooldown caps the spam: after
// a swing ends, the AI must wait at least kAiAttackCooldownTicks
// before considering another attack. 30 ticks = 0.5s.
inline constexpr int kAiAttackCooldownTicks = 30;

// Maximum screen-shake magnitude in render pixels on a KO (the last stock
// lost). The shake decays linearly to 0 over kKoShakeFrames.
inline constexpr float kKoShakeMagnitude = 12.0f;

// How long the KO shake plays in frames. ~14 frames = 0.23s; long enough to
// read as "something big happened", short enough to not nauseate.
inline constexpr int kKoShakeFrames = 14;

} // namespace ember::combat
