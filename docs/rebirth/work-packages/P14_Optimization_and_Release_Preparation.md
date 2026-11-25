# Work Package P14: Optimization and Release Preparation

| Field | Value |
|---|---|
| Purpose | Implementation plan for P14. No implementation is done by this document. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 24_PRODUCTION_ROADMAP.md, P9, P10 |
| Related | 24, 26 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## Objective
Optimize and ship.

## Entry criteria
Phases complete: P9, P10; owner approval of this phase; baseline commit and CI green (from P2 on). Starting baseline assumption: `dev/rebirth-continue` @ `2eeceae` plus approved prior phases; re-verify by running the build and tests before starting.

## Scope
Perf, packaging, crash logs, demo, playtests, store prep.

## Non-goals
Anything owned by a later phase; changes to approved shared constants without a Decision Log entry.

## Ordered task breakdown
1. Profile and optimize to budgets (20)
2. Packaging per platform
3. Release candidates
4. External playtests
5. Store assets
(each task is split into subtasks in the implementing session; list them in the PR description.)

## Expected changed areas / new files
all

## Technical references
Roadmap [24](../24_PRODUCTION_ROADMAP.md), architecture [18](../18_TECHNICAL_ARCHITECTURE.md), plus phase-relevant docs in [index](../README.md).

## Estimate and risk
8-14 FT weeks (first-release portion 6-10); see [26](../26_RISK_REGISTER.md).

## Tests
Automated: perf suite, CI release job. Manual: reference HW run, external playtest. Build: Release build with 0 warnings, all CTest suites green on Linux (and Windows/WASM from P2).

## Acceptance checklist
- [ ] 22/23/25 criteria
- [ ] No unverified claim recorded as verified; evidence files saved under `docs/rebirth/evidence/`
- [ ] Docs updated if decisions changed

## Exit criteria / gate
RC approved. Git checkpoint: tag `rebirth-phase-P14-done` only after the exit gate passes.

## Rollback / recovery
Hold release; never force-push; keep previous tags.

## Next phase dependencies
Release
