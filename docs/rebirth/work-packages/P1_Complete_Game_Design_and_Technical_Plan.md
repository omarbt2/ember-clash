# Work Package P1: Complete Game Design and Technical Plan

| Field | Value |
|---|---|
| Purpose | Implementation plan for P1. No implementation is done by this document. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 24_PRODUCTION_ROADMAP.md, P0 |
| Related | 24, 26 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## Objective
Owner-approved production bible.

## Entry criteria
Phases complete: P0; owner approval of this phase; baseline commit and CI green (from P2 on). Starting baseline assumption: `dev/rebirth-continue` @ `2eeceae` plus approved prior phases; re-verify by running the build and tests before starting.

## Scope
This documentation package; decisions D-001..D-020 reviewed.

## Non-goals
Anything owned by a later phase; changes to approved shared constants without a Decision Log entry.

## Ordered task breakdown
1. Owner review in a separate conversation
2. Resolve OPEN decisions or defer with reason
3. Update docs per review
4. Freeze constants in 00
(each task is split into subtasks in the implementing session; list them in the PR description.)

## Expected changed areas / new files
docs/rebirth/

## Technical references
Roadmap [24](../24_PRODUCTION_ROADMAP.md), architecture [18](../18_TECHNICAL_ARCHITECTURE.md), plus phase-relevant docs in [index](../README.md).

## Estimate and risk
1-2 FT weeks; see [26](../26_RISK_REGISTER.md).

## Tests
Automated: tools/check_rebirth_docs.py. Manual: Owner read-through. Build: Release build with 0 warnings, all CTest suites green on Linux (and Windows/WASM from P2).

## Acceptance checklist
- [ ] Owner written approval of next phase; zero unresolved contradictions
- [ ] No unverified claim recorded as verified; evidence files saved under `docs/rebirth/evidence/`
- [ ] Docs updated if decisions changed

## Exit criteria / gate
Approval recorded in 27_DECISION_LOG.md. Git checkpoint: tag `rebirth-phase-P1-done` only after the exit gate passes.

## Rollback / recovery
Revert doc commits; never force-push; keep previous tags.

## Next phase dependencies
P2; art spike (09) can start
