# Ember Clash - Project Memory

> Read this first if you are an AI agent picking up this project.

## What this is

A 2D 1v1 platform fighter ("Smash-style" damage-and-knockback), written from
scratch in **C++20 + SDL3 + CMake**. No game engine. Target: Windows first,
then Android, then WebAssembly (Emscripten).

## Authoritative project spec

The full project vision, architecture principles, design doc, roadmap
(M1-M8), and operating policy live in:

> `EMBER_CLASH_MASTER_ORCHESTRATOR_PROMPT.md`

That file is the design authority. This `AGENTS.md` is the
session-bootstrap memory; when the two disagree, the orchestrator doc wins
for design questions, and the actual code/tests in the repo win for "what is
true right now." If the repo and the doc disagree, the discrepancy is
itself a finding worth flagging to the owner.

## Current milestone

**M12 — Master production completion (M12-PARTIAL). V1.0 is NOT READY.** See
`docs/M12_AUDIT.md` for the full audit + the honest per-feature
classification matrix at the bottom. Headline: a real navigable
main menu / pause / rematch / settings / controls / quit-confirm
state machine; persistent Settings layer; Gamepad hot-plug layer;
Audio subsystem STUB (call sites wired, real playback MISSING);
accessibility options (screen_shake IS honored;
color_blind_palette stored but not applied). Full regression is
**194 checks GREEN**. M9 (local pack loader) and M10 (online
multiplayer) are **MISSING** (no code, not deferred). The V1.0
declaration in the first M12 audit pass was **retracted** —
the project as of this commit is not shippable. The current tag
is `m12-partial`, NOT `m12`. M11 is the previous milestone
commit (tag `m11`). The M8 commit `b1bc2a6` is preserved on the
master branch.

**M11 — Online / competitive hardening (audit complete, in-scope
work done).** M9 (local multiplayer) and M10 (online) are still
**MISSING** in the codebase, so M11 cannot harden any real online
path. The deliverable in this slice is the **deterministic
simulation foundation**: `Match::state_hash()` is a 64-bit FNV-1a
fingerprint of the full simulation state (arena, both fighters'
body/velocity/state/damage/stocks/facing/jumps/attack, match phase,
respawn timers, hit-spark, hit-stop, KO shake). The M11 test
harness (`tests/ember_test_m11.cpp`, 4 scenarios / 9 asserts, all
PASS) proves that two independent Matches driven with identical
inputs produce **bit-identical 601-tick and 600-tick hash sequences**.
The hash is the desync-detection primitive a future online
implementation would send over the wire; until M9 + M10 exist, it
has nowhere to go. See `docs/M11_AUDIT.md` for the full audit
report (including the explicit non-claims). All previous
regressions still pass: M4 AI 14/14, M5 match flow 14/14, M6-A art
12/12, M6-B anim 33/33, M6-C feel 59/59, M7-AI polish 14/14, M8
surface compile-time, M11-AUDIT determinism 9/9.

**Active sequence** (per the master orchestrator):
M6 → M7 → M11 → M12. M9 (local multiplayer) and M10 (online) are
**MISSING** in the codebase and are NOT in the active sequence; M11
online hardening is **BLOCKED** until M9+M10 actually exist, and M12
final release is **BLOCKED** until M6+M7+M9+M10+M11 are real. We do
not claim M11 "done" on online features that do not exist; we
explicitly call the gap out instead.

**M6-A work ended clean**: `CMakeLists.txt` was patched to add
`src/Gameplay/Sprite.cpp` to `EMBER_SOURCES`, the M4 and M5 test
runner OBJS lists were both updated to include `Sprite.cpp.obj` so
all three test harnesses link, and `Sprite.h`'s now-incorrect
"kSilhouette / kFighterSpriteSize" doc comment was rewritten to
match the actual palette-indexed layout.

## Build & run

```bat
:: from the repo root, in a Visual Studio "x64 Native Tools" command prompt
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
build\bin\EmberClash.exe
```

- Output binary lives at `build\bin\EmberClash.exe`; SDL3.dll is copied next
  to it by the post-build step.
- Configured with `/W4 /permissive- /std:c++20 /utf-8`.
- SDL3 3.2.10 is fetched at configure time via `FetchContent` - no manual
  SDL install is required.
- All code lives in the `ember::` namespace.
- **Debug vs Release:** keep the main game configured in `Debug`. The
  M4 + M5 test runners hard-code `/MDd` and link against the per-file
  `.obj` files that this Debug build produces. A Release reconfigure
  of the main game will break the test runners with
  `LNK2038 RuntimeLibrary mismatch` (caught and documented during M8
  audit - see `tests/run_*_test.bat` for the rationale and the
  rebuild-on-debug instruction in each header comment).
- **WebAssembly build (M8):** the Emscripten target is in
  `CMakeLists.txt` under `if(EMSCRIPTEN) ... endif()`. It compiles
  `src/main_emscripten.cpp` instead of the desktop `src/main.cpp` and
  links `-sUSE_SDL=3 -sALLOW_MEMORY_GROWTH=1 -sEXIT_RUNTIME=1`. ASYNCIFY
  is intentionally not enabled. The exact commands are in the README;
  they require the Emscripten SDK to be installed and were not
  executed in this repo's Windows-only build environment.

## Module layout

```
src/
  main.cpp                 SDL_main entry; constructs ember::App and runs it
                           (desktop only - the Web build uses main_emscripten.cpp)
  main_emscripten.cpp      Emscripten-only entry point. Compiled only when
                           CMake is configuring with an Emscripten toolchain.
                           Calls emscripten_set_main_loop() with a trampoline
                           that invokes app.step(dt). The only platform #ifdef
                           in the entire codebase lives at the boundary
                           between main.cpp and this file.
  Core/
    App.{h,cpp}            orchestrator: SDL init, owns
                           window/renderer/input/match, drives the main loop
    Config.h               tuning constants (window, timing, palette)
    Logger.{h,cpp}         thread-safe console logger (no SDL_Log)
    Math.h                 Vec2 + Aabb primitives (header-only)
    Time.{h,cpp}           capped delta-time clock
  Gameplay/
    Attack.{h,cpp}         attack table (startup/active/recovery, hitbox,
                           damage, knockback, hitstun) + active-window
                           helpers; kAttacks[] is the single source of
                           attack tuning
    AiConfig.h             AI difficulty struct (ember::ai::Difficulty)
                           + kStandard / kEasy / kHard presets; all
                           bot tuning lives here
    AiController.{h,cpp}   deterministic FSM (Idle/Approach/Attack/
                           Retreat/Recover/Defend); owns its
                           Difficulty; xorshift32 RNG; integrates
                           into Match via set_ai(idx, unique_ptr)
    CombatConfig.h         knockback formula + cap constants
    Fighter.{h,cpp}        fighter entity: state machine (Idle/Attacking/
                           Hitstun/Dead), motion step, collision, render
    InputMap.{h,cpp}       keyboard -> PlayerIntent + CombatIntent (only
                           place SDLK lives outside InputManager)
    Match.{h,cpp}          owns 2 fighters + the arena; per-frame update
                           runs fighter updates, resolves hits, respawns
  Graphics/
    Window.{h,cpp}         RAII wrapper over SDL_Window
    Renderer.{h,cpp}       RAII wrapper over SDL_Renderer; draw helpers
  Input/
    InputManager.{h,cpp}   held/pressed/released keyboard state
  Physics/
    Motion.{h,cpp}         pure per-tick step_motion(); reads
                           PhysicsConfig.h for all tuning
    PhysicsConfig.h        tuning constants (gravity, walk, jump, fast-fall,
                           air control, terminal velocity)
  UI/  (M5)
    BitmapFont.{h,cpp}     4x6 monospace font drawn with filled rects;
                           draw_text / measure_text helpers; no SDL_ttf
                           dependency, no font asset to ship
    Hud.{h,cpp}            static draw helpers for the M5 HUD: fighter
                           card (stock pips + damage %), damage floats,
                           hit spark, KO banner; format_damage helper
                           used by the headless test
  World/
    Arena.{h,cpp}          platform list + AABB collision + render
```

`Audio/` is **not yet created** - it lands in the M6-D slice
(procedural / synthesized SFX + simple SFX triggers wired into the
combat spark + UI clicks). Until then, do not create empty stub
headers there just to "reserve" the namespace. `UI/` is live in M5.

## Operating policy (from the orchestrator doc)

Key rules condensed for fast recall during a session:

* **Source of truth**: the actual files in the repo + tested behavior.
  When the spec and the code diverge, the divergence is a finding to flag.
* **No fabrication** (spec ┬¦24): never claim a model was consulted, a build
  was run, or a test passed unless it actually happened in this session.
* **When to ask the owner** (spec ┬¦30): external credentials missing,
  genuine subjective product decision, destructive action, conflicting
  requirements, or environment blocking progress. Otherwise, decide and
  continue.
* **Milestone discipline** (spec ┬¦31, ┬¦34): stop at the end of every
  milestone. Do not auto-start the next one without an explicit owner
  request.
* **Smallest coherent change** (spec ┬¦25): don't rewrite working sections
  without a strong technical reason. Preserve M1.
* **Code/test policy** (spec ┬¦25, ┬¦26): build, test, verify, then commit.
  Compilation success is not sufficient evidence of correctness.
* **Don't scatter tuning values** (spec ┬¦9): keep all gameplay-tuning
  constants centralized in `src/Physics/PhysicsConfig.h` /
  `src/Core/Config.h`. Future M2-M5 modules read from there.
* **Don't hard-code platform input into gameplay** (spec ┬¦11): all keyboard
  queries go through `InputManager`. Future gamepad/touch plug in there.

## Multi-model usage (spec ┬¦21)

The orchestrator doc names a primary "master" and several specialists.
The runtime exposes corresponding profiles:

* `gpt-5.6-sol` ŌĆö closest to the "master" role (architecture, final
  decisions, hard debugging). Use `switch_llm` when a hard call has to be
  made and an independent pass would help.
* `nemotron-3-super-120b-a12b-free` ŌĆö code review, edge cases, QA pass
  (spec ┬¦22 Rule B / Rule D).
* `nemotron-3-ultra-550b-a55b-free` ŌĆö long-horizon architecture problems
  (use sparingly).
* `laguna-s-2.1-free` ŌĆö backup.

Per spec ┬¦22 Rule E, **never pretend a model was consulted** when it
wasn't. If `switch_llm` fails or the chosen profile is rate-limited, fall
back transparently: say so internally and keep going.

## Conventions & gotchas

* **C++20**, MSVC first. No `&&` in shell commands (we're on PowerShell).
* **No traditional game engine.** No third-party deps beyond SDL3.
* **No global state.** Subsystems are owned by `ember::App` and passed by
  reference.
* **SDL3 entry point.** `main.cpp` defines `int SDL_main(int, char**)`. Do
  **not** also `#include <SDL3/SDL_main.h>` from other translation units -
  it `#define`s `main` to `SDL_main` and causes duplicate-symbol link errors.
* **Logical presentation.** The renderer targets `config::kRenderWidth` x
  `kRenderHeight`; SDL3 letterboxes it to the actual window.
* **Renderer basics.** `Renderer::draw_filled_rect` / `draw_rect_outline`
  use logical (render-space) coordinates, not raw pixel coordinates.
* **Input is platform-agnostic.** All gameplay code must query
  `InputManager` (held/pressed/released) so gamepad / touch can be added
  later without touching gameplay.
* **Tuning constants live in one place.** All physics values
  (`kGravity`, `kWalkSpeed`, `kFirstJumpVelocity`, `kDoubleJumpVelocity`,
  `kAirControl`, `kTerminalVelocity`, `kFastFallMultiplier`) are inlined
  in `src/Physics/PhysicsConfig.h`. Window/timing/palette live in
  `src/Core/Config.h`. Don't duplicate them anywhere else.
* **SDLK_* is forbidden outside InputManager + InputMap.** When
  designing new features, take a `PlayerIntent` (or an equivalent
  platform-agnostic struct) at the boundary; never reach into
  `InputManager::held(SDLK_*)` from gameplay / AI / render code.
* **Instance-member naming.** All non-static members of game classes
  end in a trailing underscore (`velocity_`, `grounded_`, `body_`,
  `jumps_remaining_`, ...). Don't drop the underscore - it will silently
  fail to compile (the parser reports "undeclared identifier" at the
  call site, not a typo diagnostic).
* **No `ember::gameplay::` sub-namespace.** M2 initially had one and
  removed it because it forced qualification across layers without
  buying any clarity. Keep types like `PlayerIntent`, `MotionState`,
  `MotionResult`, and `step_motion` directly in `ember::`.

## Testing the build

After meaningful changes:

```bat
cmake --build build --target EmberClash
```

Then either launch `build\bin\EmberClash.exe` and confirm visually that
the two fighters are sitting on the main platform and respond to WASD /
arrows, or run a non-interactive smoke test:

```powershell
Start-Process "build\bin\EmberClash.exe"
Start-Sleep -Seconds 2
Get-Process EmberClash   # should be running
Stop-Process -Name EmberClash -Force
```

For M2 the visual baseline is: dark navy background, light-blue arena
(main floor across the whole stage, two side platforms 160 px above
it, all outlined in yellow), a red P1 on the left and a blue P2 on the
right, both sitting on the main platform. Moving and jumping should
work; falling off the bottom should respawn at the starting position.

For M3 the visual baseline adds: a green hurtbox outline on each
fighter body, plus a red active-hitbox outline on whichever fighter is
currently mid-swing. Press `J`/`K` (P1) or numpad `1`/`2` (P2) on the
ground to throw a Jab or Forward Tilt, and `L` / numpad `3` in the air
for a Neutral Air. The opponent should flinch (knockback) and gain
damage %. After enough accumulated damage, a single attack will launch
them off the stage; they auto-respawn for now (the M5 respawn timer
replaces that).

For M4 the visual baseline is unchanged from M3. The difference is
that Player 2 now moves on its own even when no one is touching the
arrow keys — it's driven by the `ember::ai::AiController` state
machine. If you just let the game sit for a few seconds after launch,
P2 should walk toward P1, start swinging jabs / ftilt / nair, and
occasionally back off when it has taken damage.

For M5 the visual baseline adds:
* **Stock pips** at the top corners (P1 top-left, P2 top-right) -
  three filled rects in the player's color, one per remaining stock.
  Lost stocks are dim outlines.
* **Damage %** in the corner cards, 2x scale, 0% through 999% with
  leading-zero suppression.
* **Damage floats** above each fighter's head, 1x scale, so the
  number is visible mid-action without having to glance at the
  corner cards.
* **Hit spark** - a yellow `+` that flashes at the impact point for
  `kHitSparkFrames` (6) frames after each landed hit.
* **KO banner** - a dark band across the middle of the screen; the
  text is `GAME!` for the first `kKoBannerInitialSeconds` (1.5s),
  then `P1 WINS - PRESS R` (or `P2 WINS - PRESS R`) for the rest
  of the post-match time. Pressing `R` returns to Fighting.
* **Death and respawn**: a fighter launched below the death zone
  disappears for `kRespawnDelaySeconds` (0.5s), then reappears at
  the spawn position with a white outline (visual cue for
  invulnerability) for `kRespawnInvulnSeconds` (1.5s) more.

### Headless test harness (scratch tests)

For purely-logical changes (state machines, formulas, geometry), the
production binary's smoke test isn't enough — there's no easy way to
press specific keys at specific frames. The pattern that works on this
toolchain is to write a small `.cpp` file that includes the gameplay
headers, compile it against the same sources with
`cl /std:c++latest /EHsc ... SDL3.lib`, and assert in `main()`. The
M3 scratch test (45 cases) and M4 AI test (6 scenarios / 14 assertions)
were written this way. They both caught real bugs that the smoke
test had missed because the smoke test never actually moves fighters.

M3's test lived under `build/` and was deleted before commit. Starting
with M4, the headless tests are checked in under `tests/`:

```
tests/
  ember_test_ai.cpp    M4 AI test (6 scenarios / 14 assertions, all green)
  run_ai_test.bat      runner script (Windows) - also lives at build/run_ai_test.bat
  ember_test_m5.cpp    M5 match-flow test (6 scenarios / 14 assertions)
  run_m5_test.bat      runner script (Windows) - also lives at build/run_m5_test.bat
  ember_test_m8.cpp    M8 surface test: compile-time static_asserts on
                       App::step(double) / App::run() / App::request_exit().
                       Catches any signature drift that would silently
                       break the Emscripten build before it ships.
  run_m8_test.bat      runner script (Windows) - also lives at build/run_m8_test.bat
```

The runners are invoked as `build/run_ai_test.bat`,
`build/run_m5_test.bat`, and `build/run_m8_test.bat` (they live under
`build/` because they need to find the per-file `*.cpp.obj` files
that CMake produces there; the `tests/` sources are passed in as
command-line arguments). All test artifacts (`ember_test_*.exe`,
`*.ilk`, `*.pdb`) live under `build/` and are gitignored. The M8
runner deliberately does **not** link against the per-file `.obj`
files — it is a header-only test that just verifies the public
surface of `App` has not drifted.

### AI conventions (M4)

* `ember::ai::AiController` is a value type held by `Match` through
  `std::unique_ptr<AiController>`; it never owns fighters directly.
* The bot emits `PlayerIntent` + `CombatIntent` (the same types the
  human `InputMap` produces) so `Fighter` cannot tell whether an
  intent came from a human or the AI. This is intentional — the bot
  is just another `InputMap` from the Fighter's point of view.
* RNG is **xorshift32** (inside `AiController.cpp`) and is seeded by
  `Difficulty::seed`. Do not call `std::rand` / `<random>` directly
  inside the FSM — it would break determinism guarantees.
* All AI tuning numbers live in `src/Gameplay/AiConfig.h`. Do not
  hard-code reaction delays or attack thresholds inside
  `AiController.cpp`.
* New AI states are added by extending the `State` enum and the
  `tick_state` switch in `AiController.cpp`, and by adding the
  transition conditions in `update` (or a private helper). The
  test harness in `tests/ember_test_ai.cpp` should be extended to
  cover any new state.

### Match-flow conventions (M5)

* `ember::Match::Phase` is a 2-state machine: `Fighting` and
  `MatchOver`. Transitions only happen in one place -
  `Match::update` sets `phase_ = MatchOver` when a fighter's stock
  count drops to 0, and `App::run` calls `Match::reset()` on a
  `R` press when `phase == MatchOver`. Don't trigger resets from
  anywhere else; that creates racey transitions during the banner.
* `Match::update` runs six steps in a fixed order every tick:
  1) death-zone bookkeeping (decrement stock, start respawn
  timer), 2) drive each fighter via AI or input, 3) resolve
  hitboxes, 4) tick respawn timers, 5) check for match end, 6)
  age out the hit spark. Keep the order stable - reordering these
  can shift a stock decrement by one frame and break the harness.
* `Fighter::apply_hit` is a no-op when the target is
  invulnerable. The match flow is the only place that calls
  `Fighter::set_respawn_invuln_seconds`; production code never
  grants invulnerability by any other path.
* `Fighter::stocks_` is *per-fighter match progression*; it does
  not reset on `Fighter::respawn(arena)` (the respawn only resets
  body/damage/attack state). `Match::reset()` re-creates the
  fighters from scratch, which is the only path that restores the
  starting stock count. This matches the smash-style "stocks are
  only refreshed between matches" rule.
* All M5 tuning constants live in `src/Gameplay/CombatConfig.h`:
  `kStartingStocks`, `kRespawnDelaySeconds`, `kRespawnInvulnSeconds`,
  `kKoBannerInitialSeconds`, `kHitSparkFrames`, `kMaxHitSparks`. The
  death-zone y is in `src/World/Arena.h` (`Arena::death_zone_y()`).
  Do not hard-code any of these in `Match.cpp` / `Fighter.cpp` /
  `Hud.cpp` - tune them in CombatConfig.h / Arena.
* The HUD palette (`kHudTextColor`, `kHudStockColor`,
  `kHudStockLostColor`, `kKoBannerColor`, `kHitSparkColor`) lives
  in `src/Core/Config.h`.
* M5 deliberately avoids SDL_ttf / FreeType / any font asset. The
  4x6 monospace font in `src/UI/BitmapFont.cpp` covers the
  characters the M5 HUD actually needs: 0-9, A-Z, space, `!`, `?`,
  `+`, `-`, `:`, `.`, `%`. Add new glyphs to the `kGlyphs` table
  if a future milestone needs more (e.g. lowercase, parentheses,
  digits beyond 9).

## Git

Single branch: `master`. Commits use `openhands` as author.
Working commits to date:
* `004ab5a` - M1 foundation
* `388682a` - AGENTS.md bootstrap
* `d80ce00` - master orchestrator doc adoption
* `853062a` - M2 movement + physics + arena
* `3ed9244` - M3 combat + hitboxes (Jab, FTilt, NAir) + M2
  resolve_collision bugfix
* `c869474` - M4 AI opponent + headless test (14/14)
* `62e9dc3` - M5 match flow (stocks, respawn, KO banner, rematch,
  HUD) + headless test (14/14)
* `b1bc2a6` - M8 WebAssembly / release integration (per-milestone
  tag, App::step / run / request_exit split + main_emscripten.cpp)
* `2ed21fe` - M8 web build verified end-to-end (emcmake configure +
  build, .js + .wasm copied into web/, smoke-tested over local
  HTTP server)
* `e31b8a7` - M6-A art identity + procedural pixel-art sprite +
  arena gradient + death-zone band; M6 test harness (5 scenarios
  / 12 asserts); M4 / M5 test runners patched to link
  Sprite.cpp.obj; docs reconciled
* `5da32d8` - M6-B animation system: 6-frame Sprite strip (Idle /
  Walk / Jump / Attack / Hitstun) + headless anim::Driver +
  per-frame SDL_UpdateTexture cache. M6-B test harness (5
  scenarios / 33 asserts). Web rebuild +5 frames delta
* `b233ca2` - M6-C playability integration test: 8 scenarios /
  33 asserts; InputManager test seam (set_held_for_test /
  begin_frame_for_test); arena side-platform repositioning so
  spawn columns are clear (side_margin 80->160, side_w 240->160);
  test runner vcvars64 path fix
* `0e84cab` - M6-C features: hit-stop (kHitStopFrames=3, freezes
  motion+AI on hit), KO screen-shake (kKoShakeFrames=14,
  kKoShakeMagnitude=12px, linearly-decaying random offset
  forwarded to Renderer::set_view_offset), M3 debug outline
  removal (hitbox + hurtbox), hit-spark VFX promoted as the
  primary combat feedback. M6-C test grew to 11 scenarios /
  59 asserts. Shake tick moved to the TOP of Match::update so
  the first observable value after KO is kKoShakeFrames, not
  kKoShakeFrames-1. 136-check full regression green.
* `e54045b` - M7-AI polish: AiController::tick_defend rewritten
  to steer away from the opponent (was stand-still), removed the
  50% roll on the "enter Defend when opponent is in startup in
  range" check. Added kEasy / kStandard / kHard Difficulty
  presets in AiConfig.h (distinct seeds, distinct reaction
  windows, distinct attack frequencies, distinct recovery
  priorities). M7-AI test harness (4 scenarios / 14 asserts)
  covers preset field values + ordering, Defend backs off, Defend
  always enters, Defend exits early. 150-check full regression
  green.
* (M11 series, see `docs/M11_AUDIT.md`) - online / competitive
  hardening **AUDIT**. M9 + M10 are still MISSING so M11 cannot
  harden any real online path. The in-scope deliverable is the
  **deterministic simulation foundation**:
  - `Match::state_hash()` - 64-bit FNV-1a fingerprint of the full
    simulation state (arena, both fighters' body/velocity/state/
    damage/stocks/facing/jumps/attack, match phase, respawn
    timers, hit-spark, hit-stop, KO shake). Public API, the
    desync-detection primitive a future online implementation
    would send over the wire.
  - `Fighter::jumps_remaining()` - public accessor for the
    fighter's remaining jumps, added so the hash could include
    it without making Match a friend of Fighter.
  - `tests/ember_test_m11.cpp` - 4 scenarios / 9 asserts
    (state_hash non-zero, changes over 30 ticks, deterministic
    on repeat, 601/601 + 600/600 hash-sequence match between
    two independent runs, different seeds -> different
    sequences). All PASS.
  - `docs/M11_AUDIT.md` - the audit report. Explicitly
    non-claims that online multiplayer works, that any
    networking code exists, that rollback netcode is
    implemented, etc. 159-check full regression green.

## Test framework conventions

* **Frame / hit timing**: `Fighter::update` advances `attack_frame_`
  at the START of each tick (before hit detection). `Match::update`
  runs hit detection AFTER `Fighter::update`. Net: when a test
  observes `attack_frame_ == N` after a tick, the hit was checked
  with `attack_frame_ == N-1` (i.e. one frame earlier).
* **Jab timing** (startup=3, active=2, recovery=6): the hit lands
  on the tick where `attack_frame_` reaches 3 (the first active
  frame). After this tick, the visible `attack_frame_` is 4.
* **First-frame observable values for feel state**: the match
  contract is that the tick which SETS a feel counter (hit-stop,
  KO shake) does NOT also decrement it. The decrement happens at
  the TOP of the NEXT `Match::update` call. Tests should sample
  immediately after the tick that armed the state and expect to
  see the full duration (e.g. `kHitStopFrames`, `kKoShakeFrames`).
* **M6-C test seam**: `InputManager` has `set_held_for_test`,
  `clear_for_test`, `begin_frame_for_test` to drive headless
  Match. `begin_frame_for_test()` MUST be called BEFORE
  `set_held_for_test` for the same frame (input is sampled
  inside begin_frame).
* **M11 determinism foundation**: `Match::state_hash()` is a
  64-bit FNV-1a fingerprint of the full simulation state. The
  M11 test harness proves that two independent Matches driven
  with identical input streams + identical AI seeds produce
  bit-identical hash sequences (verified 601/601 and 600/600
  ticks). If you add a new field to any Fighter, the Arena, or
  Match that affects simulation output, you MUST fold it into
  `state_hash()` or the determinism contract silently breaks.
  Do NOT fold display-only state (renderer view offset, UI
  strings) or AI RNG internal state (two bots with the same
  seed make the same decisions regardless of RNG representation).
* **M12 Menu / App contract**: `Menu` emits an `Action` per
  frame via `consume_action()`. `App` reads it and applies the
  side effect. The Menu NEVER touches `Match` / `Fighter`
  directly. `App::update()` calls `menu_->update(input, gamepad,
  audio)` first, then handles the resulting action. The
  `blocks_simulation()` query tells `App` whether to call
  `match_->update()` this frame. To jump to a particular Menu
  state from production code use `Menu::set_state(State)`; the
  `set_state_for_test` / `selected_index_for_test` /
  `set_settings_for_test` helpers are the headless test seam
  and are mirrored in `tests/ember_test_m12.cpp`.
* **M12 Settings contract**: `Settings` is plain-old-data with
  member-initializer defaults. Add a new field by (1) declaring
  it in `Settings.h` with the default, (2) writing the
  `write_*`/`read_*` in `Settings.cpp` for the new key, (3)
  adding it to `reset_to_defaults()`, (4) adding it to
  `operator==()`, (5) (if user-facing) adding it to
  `Menu::rebuild_settings_entries()` AND the `confirm` dispatch
  in `Menu::update()`. The M12-AUDIT test round-trips every
  field through the INI parser so any missing step will trip
  the regression.
* **M12 App link list**: every `tests/run_*.bat` that links
  `App.cpp.obj` MUST also link `Settings.cpp.obj`,
  `Gamepad.cpp.obj`, `Audio.cpp.obj`, and `Menu.cpp.obj` (and
  any new subsystem the App picks up in the future). M12 added
  the four entries; M12.5-Audio and M13+ will need the same
  update when new subsystems land.
* **M12 deferred**: local pack loader (M9), online multiplayer
  (M10), real audio, input remap UI, controller haptics. All
  are documented in `docs/M12_AUDIT.md` with rationale.
