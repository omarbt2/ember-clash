# Work Package P8: Local Multiplayer

| Field | Value |
|---|---|
| Purpose | Implementation plan for P8. No implementation is done by this document. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 24_PRODUCTION_ROADMAP.md, P2, P7 |
| Related | 24, 26 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## Objective
Up to 4 local players.

## Entry criteria
Phases complete: P2, P7; owner approval of this phase; baseline commit and CI green (from P2 on). Starting baseline assumption: `dev/rebirth-continue` @ `2eeceae` plus approved prior phases; re-verify by running the build and tests before starting.

## Scope
15_LOCAL_MULTIPLAYER; N fighters; join/leave; hot-plug; FFA; humans+AI.

## Non-goals
Anything owned by a later phase; changes to approved shared constants without a Decision Log entry.

## Ordered task breakdown
1. Generalize Match to 4
2. Input slot manager
3. Join/select UI
4. Pause/disconnect handling
5. 4-player replay test
(each task is split into subtasks in the implementing session; list them in the PR description.)

## Expected changed areas / new files
src/Gameplay/Match, src/Input, src/UI

## Technical references
Roadmap [24](../24_PRODUCTION_ROADMAP.md), architecture [18](../18_TECHNICAL_ARCHITECTURE.md), plus phase-relevant docs in [index](../README.md).

## Estimate and risk
4-7 FT weeks; see [26](../26_RISK_REGISTER.md).

## Tests
Automated: 4-slot tests, input separation. Manual: 4-player hardware session (pads). Build: Release build with 0 warnings, all CTest suites green on Linux (and Windows/WASM from P2).

## Acceptance checklist
- [ ] 15 s6
- [ ] No unverified claim recorded as verified; evidence files saved under `docs/rebirth/evidence/`
- [ ] Docs updated if decisions changed

## Exit criteria / gate
Gate G8 checklist. Git checkpoint: tag `rebirth-phase-P8-done` only after the exit gate passes.

## Rollback / recovery
Keep 2-player path; never force-push; keep previous tags.

## Next phase dependencies
P9
