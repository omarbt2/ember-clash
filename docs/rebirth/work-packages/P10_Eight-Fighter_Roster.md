# Work Package P10: Eight-Fighter Roster

| Field | Value |
|---|---|
| Purpose | Implementation plan for P10. No implementation is done by this document. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 24_PRODUCTION_ROADMAP.md, P4, P5 |
| Related | 24, 26 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## Objective
Roster expansion.

## Entry criteria
Phases complete: P4, P5; owner approval of this phase; baseline commit and CI green (from P2 on). Starting baseline assumption: `dev/rebirth-continue` @ `2eeceae` plus approved prior phases; re-verify by running the build and tests before starting.

## Scope
Vesper+Isolde (first release), then Rook, Maren, Zephyr, Bastian.

## Non-goals
Anything owned by a later phase; changes to approved shared constants without a Decision Log entry.

## Ordered task breakdown
1. Per fighter: data, hook, animation, VFX, audio, balance
2. Matchup matrix update
(each task is split into subtasks in the implementing session; list them in the PR description.)

## Expected changed areas / new files
assets/data/fighters, hooks, assets/fighters

## Technical references
Roadmap [24](../24_PRODUCTION_ROADMAP.md), architecture [18](../18_TECHNICAL_ARCHITECTURE.md), plus phase-relevant docs in [index](../README.md).

## Estimate and risk
24-42 FT weeks (first-release portion 8-14); see [26](../26_RISK_REGISTER.md).

## Tests
Automated: per fighter tests; matchup sim. Manual: playtests. Build: Release build with 0 warnings, all CTest suites green on Linux (and Windows/WASM from P2).

## Acceptance checklist
- [ ] each fighter meets P4 criteria
- [ ] No unverified claim recorded as verified; evidence files saved under `docs/rebirth/evidence/`
- [ ] Docs updated if decisions changed

## Exit criteria / gate
Roster wave sign-off. Git checkpoint: tag `rebirth-phase-P10-done` only after the exit gate passes.

## Rollback / recovery
Ship fewer fighters; never force-push; keep previous tags.

## Next phase dependencies
P14
