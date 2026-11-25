# Work Package P3: Combat Engine

| Field | Value |
|---|---|
| Purpose | Implementation plan for P3. No implementation is done by this document. |
| Status | DRAFT - pending owner review |
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
1. JSON loader + validators
2. Hitbox keyframe engine
3. Grab/throw states
4. Projectile and trap entities in snapshot
5. Armor + priority + clank
6. Scenario DSL and per-move tests
(each task is split into subtasks in the implementing session; list them in the PR description.)

## Expected changed areas / new files
src/Gameplay, src/Data (new), tests/, assets/data/

## Technical references
Roadmap [24](../24_PRODUCTION_ROADMAP.md), architecture [18](../18_TECHNICAL_ARCHITECTURE.md), plus phase-relevant docs in [index](../README.md).

## Estimate and risk
6-9 FT weeks; see [26](../26_RISK_REGISTER.md).

## Tests
Automated: frame-exact scenarios, priority matrix, projectile lifetime. Manual: hitbox overlay review. Build: Release build with 0 warnings, all CTest suites green on Linux (and Windows/WASM from P2).

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
