# Work Package P2: Physics and Movement

| Field | Value |
|---|---|
| Purpose | Implementation plan for P2. No implementation is done by this document. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 24_PRODUCTION_ROADMAP.md, P1 |
| Related | 24, 26 |
| Revision date | 2026-11-26 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## Objective
One deterministic time model and complete movement set.

## Local implementation checkpoint (PARTIALLY VERIFIED; DRAFT / owner review still pending)
The current local continuation work adds a shared 60 Hz fixed-step accumulator for Desktop/Web, latched keyboard/gamepad input edges, dt-free gameplay update APIs and integer-tick respawn/invulnerability/KO-banner timers. It also adds data-driven four-sided blast zones and bounded collision substeps to prevent tunnelling through platforms. The new and existing headless suites pass with GCC 14 and Clang 17, and the 25-suite GCC matrix passes under ASan+UBSan. Full CMake/CTest remains blocked before project compilation by the unavailable SDL3 FetchContent network dependency.

The respawn deadline defect has been corrected and regression-tested: the KO tick is not counted as an elapsed wait tick; respawn follows 30 complete ticks. A controlled 20-script replay (37,200 rows) passes the positional/event criteria against equivalent blast-zone rules (0 event mismatches; max position delta 0.0001220701 px < 0.01 px). The separately tested four-sided blast zones intentionally differ from the older lower-only death boundary and remain a feature/rules review item. The originally named `2eeceae` predates intervening P2/P3 code; see [replay evidence](../evidence/p2-fixed-step-and-collision-recheck-2026-11-26.md). D-001 remains PROPOSED pending owner review/sign-off; P2 is still partial because D-001/D-002 review, Windows/WASM parity, official full build/CTest and feel gates remain open.
A further P2 increment implements one-way elevated platforms and down+jump drop-through. One-way platforms catch descent from above, allow ascent from below, and are ignored only for the initiating drop-through tick; the main floor remains solid. Focused GCC and Clang collision tests and strict Fighter translation-unit compilation pass. See [one-way/drop-through evidence](../evidence/p2-one-way-platforms-and-drop-through-2026-11-26.md).

The current increment adds airborne directional influence limited to 15 degrees from the original launch vector, an eight-tick shield-edge landing-tech buffer, and inward-facing ledge hang/climb/jump/drop on explicitly grabbable main-stage edges. A first ledge grab per airtime grants 40 ticks of invulnerability. Trivially-copyable Fighter/Match snapshots include DI reference vectors, ledge-use state, AI and training-recorder state. Restore is validated before replacing live state; a headless test verifies restored state hashes and identical next-tick hashes. Focused GCC/Clang tests and the listed existing integration regressions are being rerun after the spec-alignment changes using an SDL3 test shim; full SDL3 CMake/CTest remains unverified. See [DI/tech/ledge/snapshot evidence](../evidence/p2-di-tech-ledge-snapshot-2026-11-26.md). This does not close P2: D-001/D-002 review, full official build/CTest, Windows/WASM hash parity, and feel gates remain open.

## Entry criteria
Phases complete: P1; owner approval of this phase; baseline commit and CI green (from P2 on). Starting baseline assumption: `dev/rebirth-continue` @ `2eeceae` plus approved prior phases; re-verify by running the build and tests before starting.

## Scope
D-001 integer ticks, numeric gate D-002, `kMaxFighters`, move InputMap SDL dependency out of sim, blast zones as data, jump buffer, coyote, run/dash, drop-through, ledge, DI, snapshot skeleton, replay recorder, CI Linux+Windows+WASM determinism.

## Non-goals
Anything owned by a later phase; changes to approved shared constants without a Decision Log entry.

## Ordered task breakdown
1. Reconcile/rebaseline the replay source so it isolates D-001 from later P2/P3 code; record the required 20 scripted sequences.
2. Review and sign off (or revise) the observed 31-vs-30 respawn event-tick difference; only then close D-001.
3. Implement the numeric gate runner and decide D-002.
4. Continue the movement integration matrix and feel tuning for DI/tech/ledge; core implementations and focused edge-case tests are now present, while 4P ledge contention remains open. Keep the blast-zone/no-tunnel coverage.
5. Review the POD MatchState + state-hash implementation for cross-platform parity; value snapshots and restore/replay tests are now present, but Windows/WASM hash parity remains open.
6. Run Linux/Windows/WASM CI and full Release + CTest on real dependencies.
7. Complete feel session with 2 testers.
(each task is split into subtasks in the implementing session; list them in the PR description.)

## Expected changed areas / new files
src/Physics, src/Gameplay (Fighter, Match), src/Core (App loop interpolation), src/Input (key bindings), tests/, CMakeLists

## Technical references
Roadmap [24](../24_PRODUCTION_ROADMAP.md), architecture [18](../18_TECHNICAL_ARCHITECTURE.md), plus phase-relevant docs in [index](../README.md).

## Estimate and risk
5-8 FT weeks; see [26](../26_RISK_REGISTER.md).

## Tests
Automated: movement/collision/determinism/replay suites. Manual: feel session notes; Windows + WASM runs. Build: Release build with 0 warnings, all CTest suites green on Linux (and Windows/WASM from P2).

## Acceptance checklist
- [ ] 04 s5 met; hashes equal across 3 platforms (or Q16.16 adopted)
- [ ] No unverified claim recorded as verified; evidence files saved under `docs/rebirth/evidence/`
- [ ] Docs updated if decisions changed

## Exit criteria / gate
Gate G2 evidence committed. Git checkpoint: tag `rebirth-phase-P2-done` only after the exit gate passes.

## Rollback / recovery
Branch per sub-step; baseline tag 2eeceae kept; never force-push; keep previous tags.

## Next phase dependencies
P3
