# P0–P6 continuation evidence — 2026-11-25

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
