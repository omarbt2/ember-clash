# P3–P9 continuation evidence — 2026-11-25

| Field | Value |
|---|---|
| Purpose | Record implementation slices and verification limits for P3–P9. |
| Status | PARTIALLY VERIFIED - targeted tooling tests pass; full SDL build, CTest and phase gates remain open. |
| Revision date | 2026-11-25 |

**Overall status: IN PROGRESS.** This record describes implementation slices and
what was actually verified. It is not a phase-gate approval or a release claim.

## P3 — combat engine

Implemented in this continuation:

- Match combat now calls `Fighter::receive_hit`, so Guard, Parry, Dodge and
  invulnerability outcomes participate in the real hit-resolution path instead
  of bypassing defence with direct `apply_hit` calls.
- A non-ignored contact consumes the swing's one-hit budget. A two-frame Jab
  active window no longer deals damage once per active frame.
- Clean hits grant attacker Surge meter; active Surge scales damage and
  knockback; parries apply the configured attacker stun. Defensive contacts
  create a spark but do not trigger full hit-stop.
- Added `ember_test_combat_resolution.cpp` for single-hit and parry scenarios.

Still missing from the P3 scope: external JSON move loading, frame-keyed hitbox
tracks, grab/throw states, projectile/trap entities, armor/priority/clank,
staleness, DI, and the complete scenario DSL. The new Match integration test
has not been built in this offline environment.

## P5 — art/animation pipeline

Added `tools/validate_art_assets.py`, a dependency-free manifest validator,
with unit tests covering valid metadata, alpha-channel enforcement, frame
bounds, path traversal, licensing fields, and duplicate IDs. Added the v1
manifest schema and usage notes in `assets/README.md`.

No production fighter atlases, 47-clip character sets, VFX set, renderer atlas
loader, hitbox overlays, license review, or display screenshots were produced.
No art asset has been falsely marked as shipped. P5 remains blocked by its
P4 entry dependency and by the missing authored art.

## P6 — advanced AI and training

The previous recovery work provides four named AI presets (Easy, Standard,
Hard, Expert) and a bounded 600-tick intent recorder. This continuation
integrates recording and playback into `Match`: P1's platform-independent
motion/combat intents are captured and replayed through the normal Fighter
update path. The recorder state and captured frames are folded into the
match state hash. Free Training is exposed as a no-CPU practice option with
F5 record, F6 replay, and F7 clear controls plus a status overlay.

Still missing: the planned perception model, utility planner, personalities,
full training overlay/command coverage, matchup tests, and human calibration
at every difficulty. The Match integration has not passed a full SDL build in
this environment.

## P7 — arenas

Added three deterministic built-in layouts (Training Ring, Ember Foundry,
Sky Temple), stage-specific platform/background palettes, stage-specific spawn
positions, stage identity in the state hash, and left/right/top/bottom blast
zone checks. Added `ember_test_p7_arenas.cpp` for layout and boundary rules.

These are procedural greybox arenas, not the three finished authored stages.
The JSON arena loader, camera bounds/follow work, music hooks, visual pass,
and manual playability review remain open.

## P8 — local multiplayer

Centralized SDL event-queue ownership in `InputManager`. It now snapshots
GAMEPAD_ADDED/REMOVED instance IDs during its one polling pass; the existing
primary `Gamepad` adapter consumes those events and discovers already-attached
devices with SDL's device-ID API. This fixes the prior double-poll race where
`InputManager` could drain hot-plug events before `Gamepad` saw them. Added a
headless event-snapshot test.

This does **not** implement four simultaneous controllers. The game still has
two fighters and one primary gamepad adapter; join/leave, four-slot mapping,
player-select and four-player replay/hardware verification remain missing.

## P9 — modes and UI

Added menu flow for Versus CPU, Local 2 Players, and Free Training, followed by
selection of one of the three built-in arenas. Local 2 Players routes the
existing P2 keyboard profile to fighter 2. Free Training disables the CPU,
exposes recorder hotkeys and displays recording status. The M12 menu surface
test now asserts the new mode and arena options.

Arcade, Survival, Tutorial, profile statistics, character/team select,
complete results UI, multi-controller mode support and user testing remain
missing. This is an initial playable mode-selection slice, not the P9 exit
criteria.

## Verification record

- `python3 -m unittest discover -s tools/tests -v`: PASS, 6 art-manifest tests.
- `python3 -m py_compile tools/validate_art_assets.py tools/tests/test_validate_art_assets.py`: PASS.
- `python3 tools/check_rebirth_docs.py`: PASS (58 Markdown files at the time of this check; links, headers, constants, phases, dependencies and estimates).
- `git diff --check`: PASS at the time of this record.
- Existing pure C++ tests and new SDL-linked tests still need to be run against the actual SDL3 toolchain.
- Full CMake/SDL3 build and CTest are not claimed. The environment previously failed SDL3 FetchContent because it could not resolve `github.com`.
- No phase completion tags are created; the entry dependencies and exit gates above are not all satisfied.
