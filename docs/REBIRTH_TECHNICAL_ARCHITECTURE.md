> **SUPERSEDED** by the production bible in `docs/rebirth/` (see `docs/rebirth/README.md`). Kept for history; some values here were corrected in `docs/rebirth/00_PLAN_REVIEW_CORRECTIONS.md`.

# EMBER CLASH REBIRTH — Technical Architecture

## 1. Audited baseline (evidence, not headers)
Build: Ubuntu 24.04, g++ 13.3, CMake 4.4, Ninja, SDL3 release-3.2.10 via FetchContent. Release build: **342 steps, 0 warnings**. `ctest`: **14/14 pass** (dummy video/audio drivers). ~9k LOC in `src/`.

Legend: COMPLETE / PARTIAL / MISSING / BROKEN / UNVERIFIED. Each row says: implemented · reachable in play · visually verified · headless-tested.

| System | Status | Notes |
|---|---|---|
| Build/CTest | COMPLETE | Linux verified. Windows `.bat` scripts exist, **UNVERIFIED** here. |
| Fixed-step loop (`App.cpp`) | PARTIAL | Accumulator exists (P17); sim still takes `dt`. |
| Physics (`Motion`, `PhysicsConfig`) | PARTIAL / **inconsistent** | `Motion.h` says values scale by `dt/kFixedStep`, but `step_motion` marks `dt` `[[maybe_unused]]`. Constants are doubles in px/tick. No coyote/buffer/ledge. |
| Collision (`Arena`) | PARTIAL | AABB platforms (max 8), one layout. No drop-through/ledge/wall verified. |
| Fighter | PARTIAL | Idle/Attack/Hitstun/Dead/Guard/Dodge. Single character, test-only setters in public API. |
| Attacks (16 ids) | PARTIAL | Reachable via input after `2eeceae`; all fighters share one table; specials are parameter variants, not unique behaviors. |
| Guard/parry/dodge/Surge | PARTIAL | Implemented + tested; feel unverified. |
| Match | PARTIAL | `kFighterCount = 2` hard-coded; `state_hash()` FNV-1a with **int32-cast floats**. Not a rollback snapshot. |
| AI (`AiController`) | PARTIAL | Difficulty structs (kEasy/kStandard/kHard) exist; no EXPERT, personalities, training mode. Fairness (reaction delay) unverified. |
| Input/InputMap | PARTIAL | 13-key keyboard layout wired to combat (`2eeceae`); key-slot collision fixed; two-human tested at Match level only. Gamepad→fighter control UNVERIFIED/likely missing. |
| Haptics/Gamepad | UNVERIFIED | No device available. |
| Menus | PARTIAL | PLAY/SETTINGS/CONTROLS/QUIT only; **no local 2P entry**, no char/stage select. |
| Sprite/Animation | PARTIAL | Procedural palette-indexed rects (`Sprite.cpp`). No real art. |
| Audio | PARTIAL | Procedural synth; test passes headless; audible playback UNVERIFIED. |
| VFX | PARTIAL | Hit spark, KO shake only. |
| WebAssembly | UNVERIFIED | Prebuilt `web/EmberClash.wasm` committed; not rebuilt or run here. |
| Online | MISSING | Only `state_hash()` foundation. |
| Assets | MISSING | No image/audio files in repo. |

Debt: test-only setters in production class; `Menu.cpp` 943 lines; docs M11/M12 audits claim PASS for foundations that are not features; roster/stages/art described in RECOVERY_README as lost in the reset.

## 2. Principles (kept)
C++20 + SDL3 stays. Layering: `Core` → `Physics`/`World` → `Gameplay` (no SDL) → `AI` (uses PlayerIntent/CombatIntent only) → `Net` (new, no gameplay rules) → `Graphics/Audio/UI` (read-only view of sim).

## 3. Decisions
**D1 — One time model.** The sim advances in whole 60 Hz ticks. Remove `dt` from `step_motion`, `Fighter::update` and anything gameplay-visible; frame counts are ints. Renderer interpolates with the accumulator remainder. *Why*: rollback requires a tick-exact sim; today's mixed dt/frame code is the biggest correctness risk. *Alt*: keep variable dt — rejected.

**D2 — Fixed-point sim.** Convert sim positions/velocities to Q16.16 `int32` (wrapper `Fx`) in P2, with all tunables in one `PhysicsConfig`. *Why*: cross-platform determinism (x86/ARM/WASM) for rollback; removes the int32-cast hack in `state_hash`. *Alt*: constrained float with `-ffp-contract=off` — acceptable fallback if Fx cost is too high; decision gated by a determinism test run on Linux + WASM + Windows in P2.

**D3 — Data-driven content.** Fighters, moves, hitboxes, arenas in versioned data files (`assets/data/*.json` or a simple text format, parsed at load, validated by tests). Move behaviors that need logic (projectile, trap, counter) are named behavior hooks written in C++, not scripting.

**D4 — Simulation state is a POD snapshot.** `MatchState` = trivially copyable struct (fixed arrays, N≤4 fighters, ≤64 projectiles). Save/load = memcpy. Required for rollback and for replay tests.

**D5 — N fighters.** Replace `kFighterCount = 2` by `kMaxFighters = 4` + active count; team id per fighter.

**D6 — Input.** Per-player `InputSource` (keyboard profile or gamepad id) → `PlayerIntent+CombatIntent` → per-tick `InputFrame` (bitfield). AI and network feed the same `InputFrame`.

**D7 — Build/CI.** Add GitHub Actions (Linux build+ctest, Windows build, Emscripten build). Keep tests as plain executables under CTest.

## 4. Risks (ranked)
1. Determinism/rollback retrofit (D1+D2+D4) · High · 3–5 wks.
2. Art production for 8 fighters without verified tools in the dev environment · High.
3. Feel tuning needs human playtesting; cannot be automated.
4. Online infrastructure (relay, NAT, accounts, anti-cheat) · High · ops cost.
5. Scope: 8 fighters × ~25 animations = ~200 animations plus VFX/audio.
6. Windows/WASM/gamepad unverified.

## 5. Test strategy
Unit (collision, knockback formula), scenario (frame-exact move tests), determinism (record inputs → replay → equal hash on each platform), AI (win rates, reaction latency), perf (sim tick ≤ 0.5 ms with 4 fighters), manual checklists for feel/art/audio. A test never claims visual/audio verification.
