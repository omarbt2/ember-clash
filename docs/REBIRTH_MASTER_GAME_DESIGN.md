# EMBER CLASH REBIRTH — Master Game Design

Baseline: `dev/rebirth-continue` @ `2eeceae` (verified, 14/14 CTest suites pass headless). See Technical Architecture §1 for the audit matrix.
Companion docs: TECHNICAL_ARCHITECTURE, ART_AND_ASSET_PIPELINE, AI_DESIGN, MULTIPLAYER_DESIGN, PRODUCTION_ROADMAP. Shared constants below are authoritative for all six.

## 0. Shared constants (all docs agree)
| Item | Value |
|---|---|
| Simulation | fixed 60 Hz tick, integer frame counts, no dt in gameplay code |
| Players per match | 2 now; up to 4 by P8 |
| Launch roster / arenas | 8 fighters / 6 arenas (built 2→4→6→8 and 1→3→6) |
| AI difficulties | EASY, NORMAL, HARD, EXPERT |
| Phases | P0–P14 (see Roadmap) |
| Stock default | 3 stocks, 8:00 timer optional |

## 1. Pillars
1. **Responsive**: input-to-action under 3 frames at 60 Hz; no input swallowed by state transitions.
2. **Readable**: silhouettes, hit effects and telegraphs are legible at 1280x720 with 4 fighters.
3. **Deep but learnable**: every fighter has a clear plan, with counterplay for every tool.
4. **Fair**: ranked uses fixed stages, no hazards, server-validated results.
5. **Ember identity**: heat, ash, forging and fire-and-water elemental clashes. Fighters are original. No third-party IP.

## 2. Core loop
Spawn on stage → trade damage % → higher % means more knockback → launch the opponent past the blast zone → lose a stock → respawn with invulnerability → last fighter with stocks wins.

## 3. Mechanics (target set; ✔ = exists in baseline, ◐ = partial, ✘ = missing)
- Walk, jump, double jump, fast fall ◐ (exists; time model needs fixing, see Tech §3). Run/dash ✘, jump buffering ✘, coyote time ✘, platform drop-through ✘ (verify), ledge grab ✘, wall interaction ✘.
- Attacks ✔ 16 `AttackId`s: jab, f/u/d tilt, f/u/d smash, n/f/b/u/d air, 4 specials. Data table in `Attack.cpp`. Same moves for every fighter today (single character).
- Guard/parry/dodge ✔ (with Ember Surge meter ✔). Grabs/throws ✘ (decision: **include** grab + 2 throws in P3 because shield needs a counter).
- Knockback formula ✔ in `CombatConfig.h`. DI ✘, hit stop ✔, KO shake ✔.
- Projectile ownership/lifetime ✘ (needed for ranged archetypes, P3).
- Armor ✘ (heavy-only, P3).

## 4. Roster (original; two are built first)
| # | Name | Archetype | Weight | Hook | Build order |
|---|---|---|---|---|---|
| 1 | Kaelen Ashforge | Balanced | 100 | Ember blade; Surge empowers specials | P4 |
| 2 | Torga Stonehide | Heavy | 135 | Armored smashes, slow, huge kill power | P4 |
| 3 | Vesper Quill | Speed | 80 | Air dashes, short-range multi-hit | P10 (wave 2) |
| 4 | Isolde Cinderveil | Technical | 95 | Setup/stance switching, combo routes | P10 |
| 5 | Rook Emberwick | Ranged | 90 | Projectile zoning, weak up close | P10 (wave 3) |
| 6 | Maren Tidewarden | Trapper | 105 | Placed traps/pools with ownership and limits | P10 |
| 7 | Zephyr Kite | Aerial | 85 | Glider, strong aerials, poor ground | P10 (wave 4) |
| 8 | Bastian Wardenhold | Defensive | 115 | Counters, reflector, shield tools | P10 |
Per fighter spec lives in `docs/fighters/<name>.md` (created per fighter at its phase): bio, silhouette, palette, stats, move table (startup/active/recovery/damage/KB), animation list (Art doc §4), sound identity, matchup notes.
Balance system: data-driven move tables, a 2-fighter-at-a-time matchup test (AI vs AI round-robin, win rate must stay within 40–60% for HARD AI; flagged otherwise) plus human playtests.

## 5. Arenas
| # | Arena | Layout | Phase |
|---|---|---|---|
| 1 | Ashen Forge (competitive standard) | main stage + 2 side platforms, no hazards | P7 (first polished at P4) |
| 2 | Cinder Bridge | main + 1 center platform, narrow | P7 |
| 3 | Tidal Foundry | 3 tiered platforms | P7 |
| 4–6 | Sky Kiln, Glass Dunes, Magma Vault | casual variants may have hazards (off in ranked) | P11 |
Each arena: collision data file, blast zones, camera bounds, spawn points, parallax layers, music/ambience, perf budget (≤ 8 full-screen layers, ≤ 2 ms render at 1080p on reference HW).

## 6. Modes (shown in menu only when playable)
Training (P6) → Versus AI (exists, 1v1) → Local Versus (P8) → Free For All 3–4P (P8) → Team 2v2 (P9) → Arcade (P9) → Survival (P9) → Private Room, Online Casual (P12) → Ranked (P13).

## 7. UI/UX flow
Intro → Main menu → Mode → (Character select → Stage select → Match settings) → Match (HUD) → Pause → Results → Rematch. Online adds Lobby and Profile. All navigable by keyboard and gamepad. Rendered at 1280x720 logical, letterboxed scaling. Current menu: only PLAY / SETTINGS / CONTROLS / QUIT, i.e. most screens still to build (P9).

## 8. Audio direction
Original, synthesized in-engine today (procedural sine/square/saw, `Audio.cpp`). Target: authored stems for 1 menu + 6 arena tracks, SFX per Art doc list, separate Master/Music/SFX sliders. Audible playback has **not** been verified (headless). Needs a real device.

## 9. Monetization (later, not before P14 playtests)
Optional cosmetics only (skins, announcer, effects colors). No gameplay-affecting purchases.

## 10. Success criteria
See Roadmap acceptance table. Overall: a stranger can play a 4-player match with two fighters on one polished stage, with art, audio and menus, without help.
