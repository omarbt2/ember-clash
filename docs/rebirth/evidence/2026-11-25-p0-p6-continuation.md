# P0–P6 continuation evidence — 2026-11-25

| Field | Value |
|---|---|
| Purpose | Record the actual P0–P6 continuation work and its verification limits. |
| Status | PARTIALLY VERIFIED - P2 movement slice only; P0 build blocked; P1 and P3–P6 remain open. |
| Revision date | 2026-11-25 |

## Scope of this checkpoint

This checkpoint continues from the recovered development line at `549efcc` (which contains the earlier recovery/input commits and the Production Bible). It does not claim P0–P6 complete.

## P0 — Recovery and audit

- The local recovery bundle was cloned successfully and its history was inspected.
- The Production Bible checker passed: 56 Markdown files; links, headers, constants, phases, dependencies, and estimates.
- A clean CMake configure was attempted with Release and tests enabled. It stopped before compiling project code because FetchContent could not resolve `github.com` to download SDL3. Full CTest is therefore not verified in this environment.
- GitHub's recovery branch currently contains the status-document checkpoint, but the recovered development commits are not published there. A direct Git push cannot be completed while DNS resolution is unavailable. `master` was not changed.

## P1 — Design and technical plan

- `EMBER_CLASH_REBIRTH_MASTER_PLAN_AR.md` is the working scope reference.
- The original Production Bible still has open/proposed owner decisions. P1 is not closed until those decisions are explicitly resolved in the source docs.

## P2 — Physics and movement (partial)

Implemented in this checkpoint:
- Six-tick jump input buffer.
- Six-tick coyote-time window.
- Grounded run-speed modifier exposed in the platform-neutral motion intent (not yet bound to a player control action/UI).
- Fighter persists the new motion timers between fixed simulation ticks.
- A dedicated regression test covers grounded jump, coyote jump, double jump after coyote expiry, buffered jump on landing, run modifier speed, and contradictory horizontal input.

Verification:
- The standalone motion regression executable compiled with `-std=c++20 -Wall -Wextra -Werror` and passed.
- `git diff --check` passed.
- Full application build and CTest remain blocked by the SDL3 download failure.

Still open in P2: dash, fast-fall/drop-through/ledge recovery review, blast-zone work, DI/tech completion, replay equivalence and determinism gate, and full clean-build tests.

## P3 — Combat engine

Not complete. Existing combat foundations remain in the recovered source. The required data-driven move schema, full grabs/throws/projectile/trap/armor/priority/staleness systems, snapshot completeness, and frame-exact coverage have not been implemented/verified by this checkpoint.

## P4 — Two fully playable fighters

Not complete. Kaelen and Torga have existing baseline definitions/hooks, but complete move tables, art, balance evidence, matchup coverage, and human playtest acceptance remain open.

## P5 — Real art and animation pipeline

Not complete. Existing procedural sprite rendering is not release art. The generated concept/sprite-sheet images created during this conversation are reference prototypes only; they are not integrated, sliced/validated, animated in-engine, or cleared as production assets. D-004 art-pipeline choice remains open.

## P6 — Advanced AI and Training

Not complete. Existing basic AI behavior is not equivalent to the planned perception/situation/utility/execution architecture, four verified difficulty tiers, personalities, team/FFA targeting, or complete Training Mode. No P6 acceptance claim is made.

## Next work

1. Close remaining P2 items and add their tests.
2. Implement P3 as data-driven combat slices, each with frame-exact tests and snapshot/hash coverage.
3. Complete P4 fighter move tables and balance harness.
4. Decide and prove the P5 art pipeline with an in-engine Kaelen vertical slice before mass asset production.
5. Implement P6 AI architecture and Training Mode after combat contracts stabilize.

## Additional continuation slice — Training capture and Expert preset

Implemented after the initial P0–P2 checkpoint:
- Added `ai::kExpert` with a 14-tick reaction delay (inside the documented 12–16 tick human-floor band), 92 attack frequency, 100 recovery priority, and a distinct deterministic seed.
- Added a device-neutral `training::Recorder` that stores up to 600 pairs of `PlayerIntent`/`CombatIntent`, supports record/stop/playback/clear, and reports capacity overflow without writing out of bounds.
- Added standalone regression tests for exact intent round-trip, playback termination, 600-tick capacity, overflow handling, and difficulty ordering/bounds.

Verification run:
- `ember_test_training_recorder`: compiled with `-std=c++20 -Wall -Wextra -Werror` and passed.
- `ember_test_ai_presets`: compiled with `-std=c++20 -Wall -Wextra -Werror` and passed.
- `ember_test_rebirth_motion`: compiled with `-std=c++20 -Wall -Wextra -Werror` and passed.
- `tools/check_rebirth_docs.py`: PASS (58 Markdown files).
- `git diff --check`: PASS.

Limits:
- The recorder is a tested core utility only; no Training Setup UI, dummy behavior, frame-data overlay, hitbox overlay, or Match playback integration is claimed.
- The Expert preset exists but has not yet been wired to a difficulty-selection screen or validated through AI-vs-AI matchup simulations.
- Full CMake/CTest and runtime rendering/audio remain blocked until SDL3 is available and the application can be built.

## Additional continuation slice — P3 move-table validation and P6 difficulty wiring

Implemented:
- Added a device/render-independent structural validator for attack definitions (`attack_is_well_formed`).
- Added a table-wide regression test covering every `AttackId` from Jab through Ground Slam: valid metadata, startup/active/recovery boundaries, invalid IDs, and malformed definitions.
- Centralized difficulty-name resolution in `ai::from_name`, with safe Standard fallback for unknown persisted values.
- Wired the Expert preset into application startup and the Settings difficulty cycle (`Easy → Standard → Hard → Expert → Easy`).

Verification:
- `ember_test_attack_table`: compiled with `-std=c++20 -Wall -Wextra -Werror` and passed.
- `ember_test_ai_presets`: compiled with `-std=c++20 -Wall -Wextra -Werror` and passed, including name mapping/fallback.
- `ember_test_training_recorder`: compiled with `-std=c++20 -Wall -Wextra -Werror` and passed.
- `ember_test_rebirth_motion`: compiled with `-std=c++20 -Wall -Wextra -Werror` and passed.
- Production Bible consistency checker: PASS (58 Markdown files).
- `git diff --check`: PASS.

Limits:
- P3 remains open: this validates the current move table but does not add grabs/throws, projectiles/traps, armor/priority/staleness, or complete rollback snapshots.
- P6 remains open: Expert is selectable through the existing Settings cycle, but the AI still lacks the planned delayed-perception/utility architecture, personality profiles, team/FFA targeting, Training UI/dummy/overlays, and matchup simulation evidence.
- Full CMake/CTest and runtime validation still require SDL3 and a runnable desktop build.
