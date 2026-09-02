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

**M3 — Combat + Hitboxes (this milestone).** Three attacks ship (Jab,
Forward Tilt, Neutral Air) defined as data in `src/Gameplay/Attack.cpp`,
each with startup/active/recovery frames, a hitbox, knockback, damage,
and hitstun. The fighter is a state machine (`Idle` / `Attacking` /
`Hitstun` / `Dead`) wired into `Fighter::update`. The match resolves
AABB overlaps between the attacker's active hitbox and the defender's
hurtbox in `Match::update`. Knockback follows the design doc's
`base * (1 + damage / divisor)` formula in `src/Gameplay/CombatConfig.h`.
Debug overlays (red hitbox outline during an attack, green hurtbox
outline on the body) are drawn and will be removed when real art lands
in M6.

M2 — Movement + Physics + Arena — is complete. One M2 bug was found
and fixed during M3 testing: `Arena::resolve_collision` had its
`push_up` / `push_down` variables named the wrong way around, so the
branch that should LAND a fighter on top of a platform was the one
that BONKED, and vice versa. The M2 smoke test had not exercised the
collision path, so the bug went unnoticed until the M3 headless combat
harness tried to fall a fighter onto the floor.

M4 (Damage / Knockback / Stocks + UI for damage %, hit spark, KO) is
next. M5 is the deterministic AI opponent. We do not auto-start M4
without an explicit owner request (per orchestrator doc §34).

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
  World/
    Arena.{h,cpp}          platform list + AABB collision + render
```

`Audio/` and `UI/` are **not** created yet - we add them only when they
have content. Do not create empty stub headers just to "reserve" the
namespace.

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

### Headless test harness (scratch tests)

For purely-logical changes (state machines, formulas, geometry), the
production binary's smoke test isn't enough — there's no easy way to
press specific keys at specific frames. The pattern that works on this
toolchain is to write a small `.cpp` file under `build\` that includes
the gameplay headers, compile it against the same sources with
`cl /std:c++latest /EHsc ... SDL3.lib`, and assert in `main()`. The
M3 scratch test (45 cases, 0 failures) was written this way; it
caught an M2-era collision bug that the smoke test had missed because
the smoke test never actually moves fighters. The test code and the
intermediate `obj/` and `.exe` artifacts are deleted before commit.

## Git

Single branch: `master`. Commits use `openhands` as author.
Working commits to date:
* `004ab5a` - M1 foundation
* `388682a` - AGENTS.md bootstrap
* `d80ce00` - master orchestrator doc adoption
* `853062a` - M2 movement + physics + arena
* *(this session)* - M3 combat + hitboxes (Jab, FTilt, NAir) + M2
  resolve_collision bugfix
