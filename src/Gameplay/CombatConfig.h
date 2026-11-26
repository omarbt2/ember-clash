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
// hunting through the codebase. Durations are integer fixed-step ticks (SC-1),
// not wall-clock seconds; Match::update advances them exactly once per tick.
// -----------------------------------------------------------------------------

// Number of lives each fighter starts a match with. 3 is the Smash standard.
inline constexpr int kStartingStocks = 3;

// Match-flow durations are integer simulation ticks at SC-1's 60 Hz rate.
// A newly spawned fighter receives 1.5 s (90 ticks) of invulnerability.
inline constexpr int kRespawnInvulnTicks = 90;

// The invisible respawn wait is 0.5 s (30 full simulation ticks after KO).
inline constexpr int kRespawnDelayTicks = 30;

// The "GAME!" banner remains visible for 1.5 s (90 ticks) before the winner
// prompt takes over.
inline constexpr int kKoBannerInitialTicks = 90;

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

// --- Rebirth Phase 2: defence --------------------------------------------
// Guard: held on the ground. The first kParryWindowFrames of a guard are a
// parry window: a hit landing inside it is negated and the attacker is stunned.
inline constexpr int kParryWindowFrames   = 6;
inline constexpr int kParryAttackerStun   = 24;   // attacker punish window
inline constexpr int kShieldMax           = 600;  // shield stamina (frames*units)
inline constexpr int kShieldDrainPerFrame = 2;    // while guarding
inline constexpr int kShieldRegenPerFrame = 3;    // while not guarding
inline constexpr int kShieldDamageCost    = 40;   // per damage % absorbed
inline constexpr int kShieldBreakStun     = 70;
inline constexpr int kShieldBreakRestore  = 300;
inline constexpr double kGuardPushback    = 3.0;
// Dodge: spot dodge or short directional roll with an invulnerable window.
inline constexpr int kDodgeFrames         = 24;
inline constexpr int kDodgeInvulnStart    = 2;
inline constexpr int kDodgeInvulnEnd      = 15;   // exclusive
inline constexpr int kDodgeCooldownFrames = 40;
inline constexpr double kDodgeSpeed       = 7.0;
// --- Ember Surge ------------------------------------------------------------
// Meter 0..kSurgeMeterMax. Filled by dealing damage, taking damage (smaller),
// blocking, parrying and landing a dodge-punish. At full, the player may
// activate Surge: for kSurgeFrames their attacks deal more damage and launch
// further. The meter empties on activation and does not fill during Surge.
inline constexpr int kSurgeMeterMax       = 100;
inline constexpr int kSurgeFrames         = 300;   // 5 s
inline constexpr int kSurgeGainPerDamageDealt = 2; // meter per damage % dealt
inline constexpr int kSurgeGainPerDamageTaken = 1;
inline constexpr int kSurgeGainBlock      = 3;
inline constexpr int kSurgeGainParry      = 20;
inline constexpr int kSurgeGainDodge      = 8;
inline constexpr double kSurgeDamageMul   = 1.25;
inline constexpr double kSurgeKnockbackMul= 1.15;

} // namespace ember::combat
