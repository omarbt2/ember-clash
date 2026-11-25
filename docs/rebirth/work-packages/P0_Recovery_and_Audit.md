# Work Package P0: Recovery and Audit

| Field | Value |
|---|---|
| Purpose | Implementation plan for P0. No implementation is done by this document. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 24_PRODUCTION_ROADMAP.md,  |
| Related | 24, 26 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## Objective
Establish the verified baseline.

## Entry criteria
Phases complete: none; owner approval of this phase; baseline commit and CI green (from P2 on). Starting baseline assumption: `dev/rebirth-continue` @ `2eeceae` plus approved prior phases; re-verify by running the build and tests before starting.

## Scope
Bundle verification, clean clone, build, 14 CTest suites, evidence matrix.

## Non-goals
Anything owned by a later phase; changes to approved shared constants without a Decision Log entry.

## Ordered task breakdown
1. Verify bundle heads and commit 2eeceae
2. Build Release and run ctest headless
3. Write 00_BASELINE_AUDIT.md
(each task is split into subtasks in the implementing session; list them in the PR description.)

## Expected changed areas / new files
docs only

## Technical references
Roadmap [24](../24_PRODUCTION_ROADMAP.md), architecture [18](../18_TECHNICAL_ARCHITECTURE.md), plus phase-relevant docs in [index](../README.md).

## Estimate and risk
0-0 FT weeks; see [26](../26_RISK_REGISTER.md).

## Tests
Automated: existing 14 suites. Manual: none. Build: Release build with 0 warnings, all CTest suites green on Linux (and Windows/WASM from P2).

## Acceptance checklist
- [ ] Baseline commit, 0 warnings, 14/14 pass recorded
- [ ] No unverified claim recorded as verified; evidence files saved under `docs/rebirth/evidence/`
- [ ] Docs updated if decisions changed

## Exit criteria / gate
Audit committed. Git checkpoint: tag `rebirth-phase-P0-done` only after the exit gate passes.

## Rollback / recovery
Re-clone bundle; never force-push; keep previous tags.

## Next phase dependencies
P1
