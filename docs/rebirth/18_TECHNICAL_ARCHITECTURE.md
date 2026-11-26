# Technical Architecture

| Field | Value |
|---|---|
| Purpose | Engineering specification: layers, determinism, numeric decisions, snapshot. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 03, 04, 16, 19, 20 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

Stays C++20 + SDL3 + CMake + CTest (no engine change warranted; evidence: build clean, 14/14 tests, SDL-free simulation largely separable).
## 1. Layers and boundaries
`Core` (app, clock, config, logging) -> `Sim` (Physics, World/Arena, Gameplay/Fighter/Attack/Match, AI) with **no SDL includes** (today `InputMap.h` leaks `SDL_Keycode` - fix in P2 by moving key bindings to `Input/`) -> `Net` (new; depends on Sim snapshot only) -> `Presentation` (Graphics, Audio, UI; read-only views of sim state). Data flow: devices -> InputManager -> per-slot `InputFrame` (bitfield of SC-6 actions) -> Match::step(inputs) -> events -> presentation.
Ownership: Match owns fighters/projectiles/arena by value in `MatchState`; presentation holds const refs.
Errors: sim returns status codes; asset loading logs and falls back; logging via a small `Log` with levels; configuration centralized in `PhysicsConfig/CombatConfig` replaced by data tables loaded and validated at startup. Save data: JSON settings/profile with version field in user dir (`SDL_GetPrefPath`).
## 2. Build
CMake + Ninja; options: `EMBER_BUILD_TESTS`; presets for Linux/Windows/Emscripten; `-Wall -Wextra` already clean; add sanitizers job.
## 3. What must be deterministic
All sim state transitions (movement, collision, combat, AI decisions, RNG, timers, hashing). Presentation (camera, particles, interpolation, audio) may use floating point.
## 4. Numeric decision gate (Q16.16 vs constrained float)
Q16.16 `int32`: range +-32768 world units (stage <= 4096 px OK), precision 1/65536 px; multiply via int64 with rounding to nearest-even toward zero (define), divide via int64 shift, saturate on overflow (never wrap) - overflow becomes a test failure in debug; angles via lookup tables (1024 entries) for sin/cos, no `std::sin` in sim; collision uses integer AABB compare; conversion to float only for rendering. Cost: slight dev friction and tuning in fixed units; speed fine.
Constrained float: `float` with `-ffp-contract=off`, no x87, no `-ffast-math`, no transcendental functions in sim, identical compiler flags; risk: WASM/ARM differences, compiler updates.
**Gate (P2):** build a 100,000-tick randomized-input sim replay; run on Linux GCC, Windows MSVC/clang (CI), Emscripten (node). If constrained-float hashes match across all three on 3 builds each and with -O0/-O2, constrained float MAY be accepted; otherwise Q16.16. Fixed-point does not by itself guarantee determinism (still need integer-only ops, defined iteration order, no unordered containers, identical seeds). Decision recorded in D-002. Local partial evidence (Linux GCC/Clang, -O0/-O2, `-ffp-contract=off`, digest `3c7689ffdc9e9abb`) is recorded in [D-002 probe evidence](evidence/d002-constrained-float-probe-2026-11-26.md); Windows/WASM and multi-build checks remain open.
## 5. Hash
Replace the float-cast `state_hash` by FNV-1a/xxhash over the serialized POD snapshot (integer fields only, defined byte order, padding zeroed).
## 6. `MatchState` snapshot contents (POD, trivially copyable, fixed arrays)
Tick counter; match phase and timers (ticks); per fighter [<=4]: position, velocity, grounded, jumps, state, state tick, current attack id and frame, facing, damage, stocks, hitstun/ledge/invuln ticks, shield stamina, Surge meter and active ticks, dodge cooldown, buffered inputs, hook state (heat, armor stamina, stance, glide ticks, air-dash count, marks); projectiles [<=64]: all fields (owner, lifetime, pos, vel, reflect count, hp); traps [<=16]; hit-stop counter and per-fighter hit group history; status effects; RNG state (per match and per AI); AI controller state (reaction buffers, cooldowns, seeds, state ticks) - needed for resim of AI matches (local only, not online); stage state (hazards, moving parts; none at launch); last-input per slot (for prediction).
**Excluded:** hit sparks, KO shake offsets, camera, UI, audio, texture/animation frame index (derived from state tick), debug.
Versioning: header `{format_version, content_hash, sim_version}`; replay files carry the same; mismatches reject loading.
Snapshot completeness test: random mid-match save, mutate, restore, continue = identical hash after 600 ticks; adding a state field without adding it to the snapshot must fail a reflection-style size test (static_assert on `sizeof` + test).
## 7. Failure modes if platforms disagree
Detected by hash exchange every 30 ticks; online: end match as "desync", log inputs for analysis; offline replay: warn. Prevention: determinism CI job across platforms.
## 8. Open
Whether to adopt ECS (no - not warranted), threading (no - keep sim single-thread), scripting (no - hooks only).
