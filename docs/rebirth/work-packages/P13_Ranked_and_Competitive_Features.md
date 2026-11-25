# Work Package P13: Ranked and Competitive Features

| Field | Value |
|---|---|
| Purpose | Implementation plan for P13. No implementation is done by this document. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 24_PRODUCTION_ROADMAP.md, P12 |
| Related | 24, 26 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## Objective
Ranked and competitive services.

## Entry criteria
Phases complete: P12; owner approval of this phase; baseline commit and CI green (from P2 on). Starting baseline assumption: `dev/rebirth-continue` @ `2eeceae` plus approved prior phases; re-verify by running the build and tests before starting.

## Scope
Resim worker (model C) + anomaly detection (D), Glicko-2, leaderboards, profiles.

## Non-goals
Anything owned by a later phase; changes to approved shared constants without a Decision Log entry.

## Ordered task breakdown
1. Resim worker
2. Result intake
3. Rating system
4. Leaderboards
5. Abuse tooling
(each task is split into subtasks in the implementing session; list them in the PR description.)

## Expected changed areas / new files
backend, relay, src/Net

## Technical references
Roadmap [24](../24_PRODUCTION_ROADMAP.md), architecture [18](../18_TECHNICAL_ARCHITECTURE.md), plus phase-relevant docs in [index](../README.md).

## Estimate and risk
10-18 FT weeks; see [26](../26_RISK_REGISTER.md).

## Tests
Automated: server tests, resim equivalence. Manual: closed beta. Build: Release build with 0 warnings, all CTest suites green on Linux (and Windows/WASM from P2).

## Acceptance checklist
- [ ] results validated server-side; abuse tests
- [ ] No unverified claim recorded as verified; evidence files saved under `docs/rebirth/evidence/`
- [ ] Docs updated if decisions changed

## Exit criteria / gate
Closed beta signoff. Git checkpoint: tag `rebirth-phase-P13-done` only after the exit gate passes.

## Rollback / recovery
Disable ranked; never force-push; keep previous tags.

## Next phase dependencies
P14
