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

**M5 — Match flow (this milestone).** Each fighter starts with 3
stocks. A fighter whose body crosses the death zone this frame is
marked dead, loses one stock, and (if any stocks remain) is respawned
at the start position after `kRespawnDelaySeconds` (0.5s) with
`kRespawnInvulnSeconds` (1.5s) of invulnerability — during which
`Fighter::apply_hit` is a no-op so the respawned fighter can't be
combo'd off the platform. When one fighter's stock count hits 0,
`Match::phase` transitions to `MatchOver` and a 1.5s `GAME!` banner
shows. After that, a `P1 WINS - PRESS R` prompt is rendered and the
player rematches with `R` (handled in `App::run`). The HUD (top-left
P1 card, top-right P2 card) draws three stock pips in the player's
color plus a 2x-scaled damage %; the damage % also floats above each
fighter's head. Each landed hit briefly renders a yellow `+` spark
at the impact point for `kHitSparkFrames` (6) frames. All UI is
rendered with a hand-rolled 4x6 monospace bitmap font
(`src/UI/BitmapFont.cpp`) — no SDL_ttf dependency, no font asset to
ship. M4's AI opponent is still attached to P2 by default. Verified
by the headless M5 harness (`tests/ember_test_m5.cpp` /
`build/run_m5_test.bat`, 6 scenarios / 14 assertions) and the M4 AI
harness (14/14 still green).

M4 — AI opponent — is complete. Six FSM states, xorshift32 RNG,
`ember::ai::Difficulty` tuning struct, headless test (14/14).

M6+ covers final art, animation, audio, then the Android and
Emscripten ports. We do not auto-start M6 without an explicit
owner request (per orchestrator doc §34).

## Build & run

```bat
:: from the repo root, in a Visual Studio "x64 Native Tools" command prompt
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
build\bin\EmberClash.exe
```

- Output binary lives at `build\bin\EmberClash.exe`; SDL3.dll is copied next
  to it by the post-build step.
- Configured with `/W4 /permissive- /std:c++20 /utf-8`.
- SDL3 3.2.10 is fetched at configure time via `FetchContent` - no manual
  SDL install is required.
- All code lives in the `ember::` namespace.

## Module layout

```
src/
  main.cpp                 SDL_main entry; constructs ember::App and runs it
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

`Audio/` is still **not** created - it lands in its own milestone when
we have something to play. `UI/` is live in M5; don't create empty
stub headers in other namespaces just to "reserve" them.

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
```

The runners are invoked as `build/run_ai_test.bat` and
`build/run_m5_test.bat` (they live under `build/` because they need
to find the per-file `*.cpp.obj` files that CMake produces there;
the `tests/` sources are passed in as command-line arguments). All
test artifacts (`ember_test_*.exe`, `*.ilk`, `*.pdb`) live under
`build/` and are gitignored.

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
* `...`     - M3 combat + hitboxes (Jab, FTilt, NAir) + M2
  resolve_collision bugfix
* `...`     - M4 AI opponent + headless test (14/14)
* *(this session)* - M5 match flow (stocks, respawn, KO banner,
  rematch, HUD) + headless test (14/14)
