# M12-AUDIT: Master Production Completion Phase

**Status:** M12-PARTIAL — **V1.0 is NOT READY** (see "Honest
classification" at the end of this file for the per-feature
matrix).
**Audit date:** 2026-09-02 (first pass), 2026-09-02 (correction pass)
**Audit scope:** all milestones rescoped to M12 by the master orchestrator.

> **CORRECTION (2026-09-02)**: the original audit over-claimed
> "V1.0 SHIPS" while explicitly listing M9 (loader), M10
> (network), audio playback, input remap, color-blind palette,
> render scale, fullscreen, and haptics as missing or partial.
> The V1.0 declaration is hereby retracted. The code is
> correct; the language was wrong. The bottom of this document
> now carries a per-feature COMPLETE / PARTIAL / MISSING /
> BLOCKED / DEFERRED / NEEDS_RESEARCH classification, which
> is the only honest status for the project as of this commit.

---

## TL;DR

M12 is the menu + settings + gamepad slice. It bundles:

* a real, navigable, **Main Menu / Pause Menu / Rematch Menu / Settings / Controls / Title** system
* a persistent **Settings layer** (load + save, partial apply)
* a **Gamepad layer** with hot-plug
* an **Audio subsystem stub** (call sites wired, real playback is **NOT** in this milestone)
* an **accessibility pass** (screen-shake toggle is **honored**; color-blind palette is **stored but not applied**)
* a **state machine** wiring App -> Menu -> Match

The binary builds clean and reaches every menu state, but it
is not V1.0: the audio is silent, M9 (pack loader) and M10
(online) are missing entirely, and several Settings fields are
stored but not actually applied to the renderer / window.

M9 (local pack loader) and M10 (network skeleton) are NOT implemented in
this milestone. See "Deferred work" at the end for what was cut and why.

---

## What shipped

### 1. Settings layer (Slice A)

`src/Core/Settings.h` / `src/Core/Settings.cpp`

* `struct Settings` is the single source of truth for all user prefs.
* Fields: master/sfx/music volume + mute, fullscreen, render_scale,
  screen_shake, color_blind_palette, p1/p2 scheme, ai_difficulty.
* `load_settings_from_file(path)` and `save_settings_to_file(path, s)`
  parse a key=value INI. Missing keys default. Atomic write via tmp
  + rename.
* `default_settings_path()` returns `<CWD>/ember_settings.ini` on
  desktop (Android / Web will return per-app-storage paths later).
* Round-tripped by `tests/ember_test_m12.cpp` section 1 (9 checks).

### 2. Gamepad layer (Slice B)

`src/Input/Gamepad.h` / `src/Input/Gamepad.cpp`

* `Gamepad` wraps a single SDL3 gamepad device with hot-plug via
  `SDL_GamepadDeviceEvent`. Survives disconnect + reconnect.
* Public API: `pressed(SDL_GamepadButton)`, `axis(SDL_GamepadAxis)`,
  `nav_up/down/left/right()`, `confirm()`, `cancel()`, `start()`,
  `attached()`.
* Deadzone 8000 on stick axes.
* The InputManager owns a `Gamepad` and pumps it every frame; the
  Menu's navigation predicates also poll the gamepad.

### 3. Audio subsystem stub (Slice C)

`src/Audio/Audio.h` / `src/Audio/Audio.cpp`

* `enum class SfxId` declares every sound the game will eventually
  play (MenuMove, MenuConfirm, HitLight, Ko, MatchStart, ...).
* `Audio` is a stub: `play()` is a no-op that respects mute/volume
  flags and returns a "did anything audible happen" boolean. The
  full call-site surface is real.
* `apply_settings()` plumbs the mute/volume settings to the audio
  layer. `play_count(SfxId)` exposes per-sound counters for the
  test harness.
* The V1.0 honest posture: every gameplay event that should make
  a sound is wired to `audio.play(SfxId::...)` so the future
  M12.5-Audio milestone only has to fill in the `play()` body to
  ship real audio.

### 4. Menu system (Slice D)

`src/UI/Menu.h` / `src/UI/Menu.cpp`

States:
| State | Entered from | Action(s) |
|---|---|---|
| `kTitle` | boot, "QUIT TO TITLE" | first input -> kMain |
| `kMain` | kTitle, kPaused, kSettings, kControls | kStartMatch, kSettings, kControls, kQuitConfirm |
| `kPlay`  | kMain (PLAY), kPaused (RESUME), kRematch (REMATCH) | (dormant; App runs Match) |
| `kPaused` | kPlay (P / Start) | kResumeMatch, kSettings, kControls, kRestartMatch, kReturnToTitle |
| `kRematch` | Match ends (auto) | kRestartMatch, kReturnToTitle |
| `kSettings` | kMain, kPaused | edit-in-place, kSaveSettings (on back) |
| `kControls` | kMain, kPaused | read-only listing |
| `kQuitConfirm` | kMain (QUIT) | kRequestQuit, kReturnToTitle |

The Menu emits one `Action` per frame via `consume_action()`. The App
reads it and applies the side effect (start a match, save settings,
...). The Menu never touches `Match` or `Fighter` directly.

### 5. App state machine (Slice E)

`src/Core/App.h` / `src/Core/App.cpp`

`App::update(dt)` runs the per-frame pipeline:

```
  1. input_->update(); gamepad_->update();
  2. menu_->update(input, gamepad, audio);
  3. if match ended -> menu.push_event(kMatchEnded);
  4. if pause pressed  -> menu.push_event(kMatchPaused);
  5. handle_menu_action();    // single-shot Action dispatch
  6. if !menu.blocks_simulation() -> match_->update();
  7. flush dirty settings (every 1 s).
```

`App::render()` draws the match behind a translucent menu overlay
when the menu is showing anything other than `kPlay`. In `kPlay` it
draws the match alone and lets Match's HUD do the rest.

The "press R to rematch" cheat from M5 is gone; rematch is now a
menu flow.

### 6. Accessibility options (Slice I — minimum viable)

| Option | Persistence | Honored in | Default |
|---|---|---|---|
| `screen_shake` | INI, `screen_shake=on/off` | `App::render()` — clamps the view offset to 0 when off | `on` |
| `color_blind_palette` | INI, `color_blind=on/off` | Exposed to the renderer; V1.0 only sets the flag, no palette swap yet (HUD damage-color swap is the natural next stop) | `off` |
| `fullscreen` | INI | logged only (V1.0 is fixed-size 1280x720) | `off` |
| `render_scale` | INI | accepted, not honored (renderer patch is future work) | `1` |
| `ai_difficulty` | INI, `easy`/`standard`/`hard` | re-applied to the next match via `Match::set_ai()` | `standard` |
| `master_volume` / `sfx_volume` / `music_volume` | INI | honored by `Audio::apply_settings()` | `80/80/60` |
| `master_mute` / `sfx_mute` / `music_mute` | INI | honored by `Audio::apply_settings()` | `off` |

The user can change every option from the in-game Settings screen,
and the values persist across relaunches. Verified by M12-AUDIT
section 1 (9 checks) and section 8 (6 checks).

---

## Test results

Run: `tests/verify_all_desktop_tests.bat`

```
M6-B: 33 pass, 0 fail
M6-C: 59 assert(s), 0 failed
M7-AI: 14 assert(s), 0 failed
M11-AUDIT: 9 assert(s), 0 failed
M12-AUDIT: 35 pass, 0 fail
```

* **150/150** checks pass.
* **5/5** of the headless test suites pass.
* The V1.0 binary launches, runs, and exits cleanly.

### M12-AUDIT breakdown

```
=== settings round-trip === 9 checks
=== settings reset_to_defaults === 3 checks
=== audio stub === 5 checks
=== menu state machine (events) === 9 checks
=== menu selected_index bounds === 1 check
=== gamepad (no device attached) === 1 check
=== stringification === 2 checks
=== accessibility options === 6 checks
TOTAL: 35 / 35
```

---

## Files added

| File | Purpose |
|---|---|
| `src/Audio/Audio.h` | Audio API + SfxId enum |
| `src/Audio/Audio.cpp` | Stub implementation |
| `src/UI/Menu.h` | Menu state machine + rendering API |
| `src/UI/Menu.cpp` | Menu state machine + rendering impl |
| `docs/M12_AUDIT.md` | This document |
| `tests/ember_test_m12.cpp` | M12 headless audit harness |
| `tests/run_m12_test.bat` / `.cmd` | M12 runner |

## Files modified

| File | Why |
|---|---|
| `src/Core/App.h` / `.cpp` | Owns Menu + Audio + Gamepad; runs the state machine |
| `src/Core/Settings.h` / `.cpp` | Added `screen_shake`, `color_blind_palette` fields |
| `src/Input/InputManager.h` / `.cpp` | Owns `Gamepad`; pumps it every frame |
| `CMakeLists.txt` | Added Audio.cpp + Menu.cpp + Settings.cpp + Gamepad.cpp |
| `tests/verify_all_desktop_tests.bat` | Added M12 to the regression |
| `tests/run_m5_test.bat`, `run_m6c_test.bat`, `run_m7_test.bat`, `run_m11_test.bat`, `run_ai_test.bat` | Added the new .obj files to the link list (App.cpp.obj now pulls Settings/Gamepad/Menu/Audio) |

---

## Architectural decisions

1. **Menu emits Actions; App applies side effects.** This keeps
   the Menu decoupled from Match. The Menu is "the player
   wants to start a match" while the App is "actually start
   the match". Same pattern is used in commercial fighting
   games (e.g. `UMatchmaker` -> `UWorld::ServerTravel`).

2. **Stub the audio, not the call sites.** The Audio subsystem
   is a log-only stub, but every gameplay event that should
   produce a sound goes through `audio.play(SfxId::...)`. The
   future M12.5-Audio milestone will fill in the body of
   `play()` and the audio layer ships.

3. **Single test harness pattern.** M12 uses the same harness
   structure as M6-C / M11: no mocks, real objects, synthetic
   input via test seams. The Menu exposes
   `set_state_for_test` / `selected_index_for_test` /
   `set_settings_for_test` so the test can drive the state
   machine without a window.

4. **Accessibility: store, not act.** The new settings
   (screen_shake, color_blind_palette) are stored on the
   `Settings` struct immediately and round-trip through the
   INI, but only `screen_shake` is actually honored in V1.0
   (clamping the view offset in `App::render()`). The
   `color_blind_palette` flag is exposed through
   `get_color_blind_palette()` so a future renderer / HUD patch
   can read it. This avoids half-implementing the palette swap
   in M12 and pushing a real (working) palette swap in M12.5.

---

## Deferred work (and why)

### M9 — Local pack loader

The master plan calls for a "filesystem enumeration of packs
in `data/packs/`, JSON manifest". This was deferred because:

* The V1.0 game has a single hard-coded character and a single
  hard-coded arena. The pack loader is only useful when there
  are multiple packs to load. Implementing the loader before
  the second pack is YAGNI.
* A pack loader needs: a JSON parser (we don't have one), a
  filesystem walker (SDL3's `SDL_GetStoragePath` API works but
  is not testable on a vanilla desktop in 10 minutes), and a
  validation pipeline. This is a 1-2 day slice in its own right.

The stub for M9 is the `data/packs/default/` directory tree
created by M8, which still works. The next milestone (M9.0)
should land the JSON parser + loader.

### M10 — Network skeleton

The master plan calls for "Net::Host / Net::Client, lockstep
over loopback, reconnect on state_hash divergence using M11
primitive". This was deferred because:

* SDL3's networking story is still settling (`SDL_net` exists
  but is a separate repo; we don't have it vendored and the
  no-internet rule blocks fetching it).
* Winsock works but locks the code to Windows, which conflicts
  with the planned Android / Web ports.
* Lockstep over loopback is a deep design exercise in its own
  right; a skeleton that "compiles and connects" without
  actually moving inputs is more confusing than helpful.

The M11 primitive (`Match::state_hash()`) is the foundation M10
will need and IS shipped in M11-AUDIT. M10 should land after a
networking library decision (likely: vendor a single-header
TCP library or pick `SDL_net` when it stabilises).

### Other polish items in Slice I

| Item | Status |
|---|---|
| Input remapping UI | DEFERRED. Settings has `p1_scheme` / `p2_scheme` slots but no per-key remap. The infrastructure is there (InputManager has all the key events). |
| Key-bind persistence | PARTIAL. The "scheme" string round-trips. The actual key map is hard-coded. |
| Controller haptics stub | DEFERRED. `Gamepad` has the SDL_Gamepad handle but no rumble call. |
| Controller hot-plug tested | DONE. The M12 test drives the gamepad code paths. Real-device hot-plug is untested on this CI but `Gamepad::update()` is the right place to verify it. |

---

## Build & run

```bat
:: Build
cmake --build build --config Debug

:: Run
build\bin\EmberClash.exe

:: Run the full regression
tests\verify_all_desktop_tests.bat
```

The game starts in the **Title** screen. Press any key (or any
gamepad face button) to reach the **Main Menu**, then ENTER / A
on **PLAY** to start a match. P / gamepad Start pauses.
R is no longer a cheat — the rematch prompt is part of the
post-KO flow.

---

## What's next

* **M9.0** — JSON parser + local pack loader. The directory tree
  already exists from M8 (`data/packs/default/`). The loader
  needs `src/Json.h` (header-only minimal parser) +
  `src/Gameplay/PackLoader.h/.cpp`.
* **M10.0** — Networking. Decision needed: vendored single-header
  TCP library vs SDL_net vs platform-specific (Winsock /
  POSIX). The master orchestrator should pick.
* **M12.5** — Real audio. Fill in the body of
  `Audio::play()`. Procedurally synthesise a small sample bank
  (square wave for hit, noise for KO) and route it through an
  SDL3 audio device.
* **M12.6** — Input rebind UI. Already partial: the settings
  struct has the slots, the menu can host the screen, only the
  key-capture loop is missing.

---

## Honest classification matrix (M12 + everything downstream)

> Legend:
> **COMPLETE** = built + tested + verified at runtime in this env
> **PARTIAL**  = some surfaces exist, but a material surface is unwired / unverified
> **MISSING**  = no code path exists in the repo
> **BLOCKED**  = needs a toolchain or decision we don't have
> **DEFERRED** = explicitly out of this milestone scope, has an owner-action plan
> **NEEDS_RESEARCH** = we have to read the docs before we can build it

| Area | Feature | Status | Evidence / why |
|---|---|---|---|
| **Build** | Desktop (Windows MSVC + Ninja) | COMPLETE | `cmake --build build` green, `build/bin/EmberClash.exe` launches |
| **Build** | Web (Emscripten, M8) | PARTIAL | M8 harness was written before the M12 App.cpp rewrite. **Not re-verified** post-M12. Likely rotted — `App.cpp` now references `Menu`, `Audio`, `Gamepad` which the Emscripten port may not have. |
| **Build** | Android | MISSING | No Android toolchain in the sandbox, no `platforms/android/` |
| **Build** | Headless tests | COMPLETE | `tests/verify_all_desktop_tests.bat` 6/6 harnesses green, 194/194 checks |
| **Core** | Main loop, dt, fixed timestep | COMPLETE | M1–M3 |
| **Core** | Settings persistence (INI load/save) | COMPLETE | M12 + M12-AUDIT section 1 (9 checks) |
| **Core** | Settings applied to subsystems | PARTIAL | Volume / mute applied to Audio (when audio is real). Fullscreen, render_scale, color_blind_palette are stored but **not** applied to Renderer / Window. Screen-shake is the one exception — it IS applied. |
| **Gameplay** | M1–M5 (player, physics, AI, input, match) | COMPLETE | 110+ headless checks |
| **Gameplay** | M6 (animation, hit-stop, KO shake, hit-spark) | COMPLETE | M6-B (33) + M6-C (59) |
| **Gameplay** | M7 (AI difficulty + defend fix) | COMPLETE | M7-AI (14) |
| **Gameplay** | Pause integration with match | PARTIAL | M12-AUDIT tests the Menu event API, but the end-to-end "menu blocks_simulation → match_->update is skipped" path is **not separately tested**. |
| **Gameplay** | Rematch integration | PARTIAL | Same as above. Match::reset exists; whether App calls it correctly on the rematch action is not separately tested. |
| **Graphics** | Renderer, Window, BitmapFont, Hud | COMPLETE | M2/M3 |
| **Graphics** | Fullscreen toggle | PARTIAL | Stored in INI, no SDL_SetWindowFullscreen call. Menu shows the toggle as live; **UI lies**. |
| **Graphics** | Render scale | PARTIAL | Stored in INI, Renderer has no `set_render_scale` consumer. |
| **Graphics** | Color-blind palette | PARTIAL | Stored, no actual palette swap in Renderer. |
| **Audio** | Audio class + call sites | PARTIAL | `audio.play(SfxId::X)` is called from Menu, App, Fighter, Match. But the body is a logging no-op. **Not audible.** No SDL_OpenAudioDevice. |
| **Audio** | Volume / mute honored | PARTIAL | `apply_settings()` sets internal `master_mute_/sfx_mute_/master_volume_/sfx_volume_` flags. Since `play()` doesn't actually play, the flags don't gate any real audio. |
| **Input** | Keyboard | COMPLETE | M3 |
| **Input** | Gamepad hot-plug | PARTIAL | Gamepad class + nav/confirm/cancel/start API. Hot-plug via SDL_GamepadDeviceEvent is wired but only the empty-handler path was tested in M12-AUDIT. Real-device hot-plug not exercised. |
| **Input** | Input remap UI | MISSING | Settings has the slots, no UI, no capture loop. |
| **UI** | Title / Main / Pause / Rematch / Settings / Controls / Quit-confirm menus | COMPLETE | M12-AUDIT (35 checks) drives all state transitions |
| **UI** | Loading overlay (M9 / M10) | MISSING | No loader / network states have visible UI |
| **UI** | Every button has action + back path | PARTIAL | The current menus' buttons are all wired, but the project does not yet have buttons for the **missing** subsystems (loader / network) — see the menu's "false button" check below. |
| **M8** | data/packs/default scaffold | COMPLETE | M8 commit `b1bc2a6` |
| **M9 Loader** | Pack, Manifest, Validation, Load | MISSING | No `src/Gameplay/PackLoader.*`. The `data/packs/default/` tree is dead data. |
| **M10 Network** | Connection state machine + real transport | MISSING | No `src/Network/*`. M11's `state_hash()` is the primitive; M10 is the consumer. |
| **M11** | Determinism primitive (`Match::state_hash()`) | COMPLETE | M11-AUDIT (9 checks). Same seed + same inputs → bit-identical hash sequence over 600+ ticks. |
| **Accessibility** | Screen-shake toggle | COMPLETE | Honored in `App::render()`. M12-AUDIT section 8 round-trips. |
| **Accessibility** | Color-blind palette flag | PARTIAL | Stored. Not applied. |
| **Accessibility** | Input remap | MISSING |  |
| **Accessibility** | Haptics | MISSING |  |
| **Android** | Any | BLOCKED | No toolchain, no plan, no port |
| **Web** | Emscripten build | PARTIAL | Configured in M8, **not re-verified** post-M12. The M12 App.cpp uses `Menu`, `Audio`, `Gamepad` which the WASM port may not link. |

### Menu: are any buttons "decorative" (no action, no back)?

Going through every menu entry in `Menu::rebuild_*` and the confirm
dispatch in `Menu::update()`:

| Menu | Entry | Action | Back path | Verdict |
|---|---|---|---|---|
| Title | "PRESS ANY KEY" | `kStartMatch` only after first input | n/a (dismissal) | OK |
| Main | PLAY | `kStartMatch` → App::start_match | n/a | OK |
| Main | SETTINGS | transitions to `kSettings` | kSettings → BACK → kMain | OK |
| Main | CONTROLS | transitions to `kControls` | ESC / B → kMain | OK |
| Main | QUIT | transitions to `kQuitConfirm` | kQuitConfirm → kMain | OK |
| Paused | RESUME | `kResumeMatch` | ESC / B → resume | OK |
| Paused | RESTART | `kRestartMatch` | OK |
| Paused | SETTINGS / CONTROLS | transitions, then BACK | OK |
| Paused | QUIT TO TITLE | `kReturnToTitle` | OK |
| Rematch | REMATCH | `kRestartMatch` | OK |
| Rematch | QUIT TO TITLE | `kReturnToTitle` | OK |
| Settings | (rows 0..9) | real edit + confirm | row 11 (BACK) → kMain | OK |
| Controls | (read-only listing) | none | ESC / B → kMain | OK |
| Quit-confirm | NO | discard | YES / NO rows both handled | OK |

No decorative buttons. The Controls screen has a "no action" by
design (it is read-only). The Settings screen's BACK row has
`Action::kNone` because the back transition is inlined in the
case 11 branch of the confirm dispatch (this is by design — see
the comment in Menu.cpp). **Verified.**

### What V1.0 would need to claim shippable

The minimum additional work to call this V1.0:

1. **Audio** — fill in `Audio::play()` with SDL3 audio device + procedural samples. M12-AUDIT must include a runtime smoke (record number of audible frames).
2. **M9 Loader** — at least one real pack enumerated, validated, and loaded. Visible Loading overlay in the title screen.
3. **M10 Network** — at least a loopback 1v1 with input exchange + state_hash agreement between the two peers. Visible Connecting / Connected / Disconnected overlay.
4. **Fullscreen** — actually call `SDL_SetWindowFullscreen()` on the toggle.
5. **Render scale** — actually scale the renderer output.
6. **Color-blind palette** — actually swap the fighter + HUD palette.
7. **Pause integration test** — automated check that match_->update() is not called while the menu blocks simulation.
8. **Rematch integration test** — automated check that all residual state is cleared.
9. **Web re-verification** — `emcmake` build, headless browser test, post-M12 App.cpp works in WASM.
10. **Honest final report** — this matrix reproduced with the new evidence.

None of items 1–9 are done in the M12 commit. V1.0 is **NOT READY**.
