# Game Design Master

| Field | Value |
|---|---|
| Purpose | High-level rules of the game. Each feature lists purpose, rules, feedback, counterplay, interactions, edge cases, dependencies, tests. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 03, 04, 05, 07 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

Detailed numbers live in 03 (combat), 04 (movement); all values are initial tuning targets unless tagged VERIFIED.

## 1. Match structure
Intro (120 ticks: READY 60, FIGHT 60, VERIFIED constants) -> Fighting -> MatchOver -> Results -> Rematch. Default 3 stocks [SC-9]; optional 8:00 timer, sudden death on tie (PROPOSED). Modes: see 13/25.

## 2. Features
| Feature | Purpose / rules | Feedback | Counterplay | Edge cases | Depends | Tests |
|---|---|---|---|---|---|---|
| Damage % & knockback | Damage raises launch distance. Formula in 03 | % counter colour steps at 50/100/150 | DI, shield, dodge, weight | % cap 999 (VERIFIED) | combat | formula tables |
| Stocks/KO | Leaving blast zones loses a stock | KO banner, shake (VERIFIED 14 f), SFX | recovery moves | simultaneous KO -> both lose a stock; last-stock tie -> sudden death | match | KO scenario tests |
| Respawn | 0.5 s delay (30 ticks), 1.5 s invuln (90 ticks) VERIFIED as seconds; ticks PROPOSED | halo platform | invuln ends early on attack | camping spawn | match | timer tests |
| Blast zones | 4 per arena as data (MISSING today) | warning flash at 300 px | - | camera clamp | arena data | KO tests |
| Ground/air combat | 16 attack ids (VERIFIED) | hit stop 3 f (VERIFIED) | spacing, shield | landing lag | combat | frame tests |
| Guard/parry/dodge | Shield stamina 600; parry first 6 f; dodge 24 f invuln 2-14 (VERIFIED constants) | flash, SFX | grabs beat shield | shield break stun 70 f | combat | scenario |
| Grab/throw | Beats shield; 6 throws/pummel clips [SC-7] | grab SFX | spot dodge, jump-out | mutual grab -> both whiff, 20 f pushback | combat P3 | grab tests |
| Ember Surge | Meter 0-100 builds on damage/block/parry/dodge; 300 ticks buff: damage x1.25, KO x1.15 (VERIFIED), plus per-fighter twist (05) | aura, music duck | meter denial, avoid trades | meter on KO resets to 50% (PROPOSED) | combat | meter tests |
| Platforms | Pass-through from below, drop-through with down+jump (MISSING) | - | - | simultaneous | physics | collision |
| Ledge | Snap within 24 px, 40 f invuln limited to once per airtime stall (MISSING) | hang anim | edge-guard, ledge trump | 4P ledge contention: first-come, others pushed | physics | ledge tests |
| FFA | 3-4 players, stocks, no team damage | colour-coded | - | sudden death with 3 | P8 | FFA sim |
| Team 2v2 | Friendly fire OFF default (setting) | team tint | - | partner KO revives not allowed | P9 | team tests |
| Training | Dummy, frame data, record/replay (12) | overlay | - | - | P6 | - |
| Rematch | Immediate restart, same settings | - | - | - | match | flow test |
| Progression | First release: profile stats only (wins, KOs); cosmetic unlocks post-launch | - | - | - | P9 | - |

## 3. Match settings
Stocks 1-5, timer off/3/5/8 min, damage ratio 0.5-2.0, teams, stage hazards (casual only), Surge on/off.

## 4. Onboarding
First launch: 90-second interactive tutorial (move, jump, attack, shield, KO) with skippable prompts; controller-aware prompts (13).

## 5. Competitive fairness rules
Stage list restricted; hazards off; no random; deterministic simulation; versioned balance patches (29).
