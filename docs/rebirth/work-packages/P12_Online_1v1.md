# Work Package P12: Online 1v1

| Field | Value |
|---|---|
| Purpose | Implementation plan for P12. No implementation is done by this document. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 24_PRODUCTION_ROADMAP.md, P2, P3, P4, P9 |
| Related | 24, 26 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## Objective
Online 1v1 unranked.

## Entry criteria
Phases complete: P2, P3, P4, P9; owner approval of this phase; baseline commit and CI green (from P2 on). Starting baseline assumption: `dev/rebirth-continue` @ `2eeceae` plus approved prior phases; re-verify by running the build and tests before starting.

## Scope
16_ONLINE_NETCODE: rollback, transport, relay, rooms, rematch, disconnect.

## Non-goals
Anything owned by a later phase; changes to approved shared constants without a Decision Log entry.

## Ordered task breakdown
1. Snapshot save/restore speed work
2. Input history/rollback
3. Transport + relay
4. Room flow
5. Netsim harness
6. Backend prototype
(each task is split into subtasks in the implementing session; list them in the PR description.)

## Expected changed areas / new files
src/Net (new), relay service, backend

## Technical references
Roadmap [24](../24_PRODUCTION_ROADMAP.md), architecture [18](../18_TECHNICAL_ARCHITECTURE.md), plus phase-relevant docs in [index](../README.md).

## Estimate and risk
14-24 FT weeks; see [26](../26_RISK_REGISTER.md).

## Tests
Automated: netsim 1000 matches, desync fuzz. Manual: two real machines at 100/150 ms. Build: Release build with 0 warnings, all CTest suites green on Linux (and Windows/WASM from P2).

## Acceptance checklist
- [ ] 16 s9
- [ ] No unverified claim recorded as verified; evidence files saved under `docs/rebirth/evidence/`
- [ ] Docs updated if decisions changed

## Exit criteria / gate
Gate G12. Git checkpoint: tag `rebirth-phase-P12-done` only after the exit gate passes.

## Rollback / recovery
Delay-based fallback; never force-push; keep previous tags.

## Next phase dependencies
P13
