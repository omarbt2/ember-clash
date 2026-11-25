# Baseline Verification and Evidence Matrix

| Field | Value |
|---|---|
| Purpose | Records exactly what was inspected, built and tested, and classifies existing systems by evidence. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 00_PLAN_REVIEW_CORRECTIONS.md, 18_TECHNICAL_ARCHITECTURE.md |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Baseline

| Item | Result |
|---|---|
| Source baseline | branch `dev/rebirth-continue`, commit `2eeceaea2452164e4fe8c064d8e0572ce6678b5e` - VERIFIED present in the supplied bundle and in a fresh clone |
| Ancestry | `2eeceae` -> `9f05a04` (merge of master) -> `7fddfde` (recovery snapshot) - VERIFIED via `git log` |
| Prior docs commit | `644bd8d` on `docs/rebirth-plan` (docs only) - VERIFIED; this package is built on top of it on branch `docs/rebirth-production-bible` |
| Source branch unchanged | `dev/rebirth-continue` still resolves to `2eeceae` - VERIFIED |
| Bundle validity | `git bundle verify` on `ember-clash-rebirth-plan.bundle`: complete history - VERIFIED; original `ember-clash-rebirth.bundle` listed heads and cloned cleanly - VERIFIED |
| Working tree | clean at start of this task - VERIFIED |
| Toolchain | Ubuntu 24.04, g++ 13.3.0, CMake 4.4.5 (pip), Ninja, SDL3 `release-3.2.10` fetched by CMake |
| Build | Release, 342 steps, 0 compiler warnings - VERIFIED (earlier session; build dir reused and `ctest` re-run in this session) |
| Tests | `ctest`: 14/14 pass with `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy` - VERIFIED |
| Not verified | Windows build, WebAssembly rebuild/run, real display, audible audio, gamepad hardware, any network |
| Discrepancies vs reports | None in hashes/branches. One earlier-plan claim corrected, see 00_PLAN_REVIEW_CORRECTIONS.md |

Test suites (14, about 4,100 lines): ai, m5, m6, m6b, m6c, m7, m8, m11 (determinism), m12, m125_audio, m14, p15_playability, p17_playability, rebirth_input. They are headless logic tests. They cannot prove visuals, audible sound, hardware input or feel.

## 2. Evidence matrix

Columns: Implemented / Reachable in play / Visually verified / Headless-tested.

| System | Implemented | Reachable | Visual | Tested | Notes |
|---|---|---|---|---|---|
| Fixed-step loop | VERIFIED | VERIFIED (read of `App::run`) | UNVERIFIED | PARTIALLY (p17) | Accumulator at 1/60 s, max catch-up ticks cap, render once per frame, **no render interpolation** |
| Motion (`step_motion`) | VERIFIED | via Fighter | UNVERIFIED | PARTIALLY | `dt` parameter unused by design; callers pass constant 1/60 |
| Seconds-based timers | VERIFIED | yes | - | partly | `respawn_timer_`, `ko_banner_timer_`, invuln seconds are `double` seconds decremented by dt |
| Collision / Arena | VERIFIED | yes | UNVERIFIED | PARTIALLY | One layout: full-width floor + 2 side platforms; **single kill plane** at `y > H+200`; no side/top blast zones; no drop-through/ledge/wall |
| Fighter states | VERIFIED | yes | UNVERIFIED | PARTIALLY | Idle, Attacking, Hitstun, Dead, Guard, Dodge |
| 16 attack ids | VERIFIED | VERIFIED after `2eeceae` (13-key layout) | UNVERIFIED | PARTIALLY | One shared move table; specials are parameterized strikes, not unique behaviors |
| Guard/parry/dodge/Surge | VERIFIED | yes | UNVERIFIED | PARTIALLY | Constants in `CombatConfig.h` (parry window 6 f, dodge 24 f, Surge 5 s) |
| Grabs/throws, projectiles, armor, DI, ledge | MISSING | - | - | - | |
| Match | VERIFIED | yes | UNVERIFIED | yes | `kFighterCount = 2`; `ai_[i]`; state hash uses FNV-1a |
| `state_hash()` | VERIFIED | test only | - | m11 | Header comment says float values are cast to int32; the code calls `mix_f32/mix_f64` on some fields (arena death zone, KO banner, respawn timers). Comment and code disagree; raw float bits are not cross-compiler safe. Needs review in P2 |
| AI | VERIFIED | yes (Versus AI) | UNVERIFIED | PARTIALLY | `Difficulty{reaction_delay_ticks, attack_frequency, recovery_priority, defensive_threshold_percent, jump_aggression, seed}`. kEasy delay 70, kHard delay 18 ticks. No EXPERT, personalities, training mode |
| InputMap/keyboard | VERIFIED | yes | - | rebirth_input | 13 key bindings; keycode-aliasing fix in `2eeceae`; two-human input tested at Match level |
| Gamepad / haptics | PARTIALLY (code exists in `Input/`) | UNVERIFIED | - | none | No hardware; whether a pad can drive a fighter end-to-end is UNVERIFIED |
| Menus | VERIFIED | yes | UNVERIFIED | none | PLAY/SETTINGS/CONTROLS/QUIT; no mode, character or stage select; no local 2P entry |
| Sprites | VERIFIED (procedural palette-indexed rectangles) | yes | UNVERIFIED | - | No authored art; no image files in repo |
| Audio | VERIFIED (procedural synth) | yes | - | m125 | Audible playback UNVERIFIED |
| VFX | PARTIALLY | yes | UNVERIFIED | - | Hit spark (4 max, 6 f), KO shake |
| WebAssembly | PARTIALLY | UNVERIFIED | UNVERIFIED | none | Prebuilt `web/EmberClash.wasm` committed, not rebuilt/run here |
| Online | MISSING | - | - | - | Only the `state_hash()` fingerprint exists |
| Assets/data files | MISSING | - | - | - | All content is in C++ tables |

## 3. Architecture observations
- SDL is confined mostly to `Core`, `Graphics`, `Input`, `Audio`, `UI`; `Gameplay` includes `Gameplay/InputMap.h` which uses `SDL_Keycode` (UI/Input leak into gameplay layer) - PARTIALLY VERIFIED.
- Test-only setters (`set_*_for_test`) live in the production `Fighter` class.
- `Menu.cpp` is 943 lines (largest file); `App.cpp` 644.
- Combat tables are `static` C++ arrays in `Attack.cpp`.
