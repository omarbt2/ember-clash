# Work Package P7: Three Complete Arenas

| Field | Value |
|---|---|
| Purpose | Implementation plan for P7. No implementation is done by this document. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 24_PRODUCTION_ROADMAP.md, P3, P5 |
| Related | 24, 26 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## Objective
Three competitive arenas with real art.

## Entry criteria
Phases complete: P3, P5; owner approval of this phase; baseline commit and CI green (from P2 on). Starting baseline assumption: `dev/rebirth-continue` @ `2eeceae` plus approved prior phases; re-verify by running the build and tests before starting.

## Scope
Arena data loader, blast zones/camera, arenas 1-3, music hooks.

## Non-goals
Anything owned by a later phase; changes to approved shared constants without a Decision Log entry.

## Ordered task breakdown
1. Arena schema loader
2. Camera system
3. Arena 1
4. Arenas 2-3
5. Collision fixtures
(each task is split into subtasks in the implementing session; list them in the PR description.)

## Expected changed areas / new files
src/World, src/Graphics, assets/arenas

## Technical references
Roadmap [24](../24_PRODUCTION_ROADMAP.md), architecture [18](../18_TECHNICAL_ARCHITECTURE.md), plus phase-relevant docs in [index](../README.md).

## Estimate and risk
5-8 FT weeks; see [26](../26_RISK_REGISTER.md).

## Tests
Automated: collision/KO/spawn tests. Manual: visual and readability pass. Build: Release build with 0 warnings, all CTest suites green on Linux (and Windows/WASM from P2).

## Acceptance checklist
- [ ] 07 tests
- [ ] No unverified claim recorded as verified; evidence files saved under `docs/rebirth/evidence/`
- [ ] Docs updated if decisions changed

## Exit criteria / gate
3 arenas playable. Git checkpoint: tag `rebirth-phase-P7-done` only after the exit gate passes.

## Rollback / recovery
Default arena remains; never force-push; keep previous tags.

## Next phase dependencies
P8, P11
