# Work Package P3: Combat Engine

| Field | Value |
|---|---|
| Purpose | Implementation plan for P3. No implementation is done by this document. |
| Status | IN PROGRESS - PARTIALLY VERIFIED (combat resolution, regression coverage, and schema-1 move catalog loader started; phase gate not met) |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 24_PRODUCTION_ROADMAP.md, P2 |
| Related | 24, 26 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## Objective
Extensible combat engine.

## Entry criteria
Phases complete: P2; owner approval of this phase; baseline commit and CI green (from P2 on). Starting baseline assumption: `dev/rebirth-continue` @ `2eeceae` plus approved prior phases; re-verify by running the build and tests before starting.

## Scope
Data-driven moves (19), hitbox keyframes, hooks, projectiles, traps, grabs/throws, armor, priority/clank, staleness, combat tests, hit-stop/DI.

## Non-goals
Anything owned by a later phase; changes to approved shared constants without a Decision Log entry.

## Ordered task breakdown
1. JSON loader + validators — PARTIALLY VERIFIED: schema-1 parser/validator and 16-move starter catalog exist; Match now loads the catalog and maps the currently implemented `AttackId` roster. The stated 19-move target does not match the current 16-entry `AttackId` table.
2. Hitbox keyframe engine — PARTIALLY VERIFIED: inclusive windows, facing-mirrored geometry, AABB and true circle-vs-AABB are wired into Match fixed-tick hit resolution. Starter-move frame scenarios and content payload/geometry tests pass on GCC and Clang; full SDL-linked combat integration CTest remains unverified due to incomplete runner SDL headers.
3. Grab/throw states
4. Projectile and trap entities in snapshot
5. Armor + priority + clank
6. Scenario DSL and per-move tests
(each task is split into subtasks in the implementing session; list them in the PR description.)

## Latest implementation evidence

Runtime loader-to-Match integration and content fingerprint/snapshot compatibility: [P3 runtime integration evidence](../evidence/p3-runtime-hitbox-integration-2026-11-26.md).

## Expected changed areas / new files
src/Gameplay, src/Data (new), tests/, assets/data/

## Technical references
Roadmap [24](../24_PRODUCTION_ROADMAP.md), architecture [18](../18_TECHNICAL_ARCHITECTURE.md), plus phase-relevant docs in [index](../README.md).

## Estimate and risk
6-9 FT weeks; see [26](../26_RISK_REGISTER.md).

## Tests
Automated: strict schema/parser and hitbox-evaluator tests on GCC and Clang, starter-catalog validation, frame-exact scenarios, priority matrix, projectile lifetime. Manual: hitbox overlay review. Build: Release build with 0 warnings, all CTest suites green on Linux (and Windows/WASM from P2).

## Acceptance checklist
- [ ] 03 s16 satisfied
- [ ] No unverified claim recorded as verified; evidence files saved under `docs/rebirth/evidence/`
- [ ] Docs updated if decisions changed

## Exit criteria / gate
Gate G3. Git checkpoint: tag `rebirth-phase-P3-done` only after the exit gate passes.

## Rollback / recovery
Feature flags per mechanic; never force-push; keep previous tags.

## Next phase dependencies
P4, P6, P7
