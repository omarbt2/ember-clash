# Work Package P4: Two Fully Playable Fighters

| Field | Value |
|---|---|
| Purpose | Implementation plan for P4. No implementation is done by this document. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 24_PRODUCTION_ROADMAP.md, P3 |
| Related | 24, 26 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](../00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## Objective
Kaelen and Torga fully playable with unique specials.

## Entry criteria
Phases complete: P3; owner approval of this phase; baseline commit and CI green (from P2 on). Starting baseline assumption: `dev/rebirth-continue` @ `2eeceae` plus approved prior phases; re-verify by running the build and tests before starting.

## Scope
All moves from fighters/*.md for two fighters, hooks forge_heat and armor_frames, balance pass with AI-vs-AI.

## Non-goals
Anything owned by a later phase; changes to approved shared constants without a Decision Log entry.

## Ordered task breakdown
1. Author move data
2. Implement hooks
3. Matchup sim tool
4. Balance iterations
5. Playtests
(each task is split into subtasks in the implementing session; list them in the PR description.)

## Expected changed areas / new files
assets/data/fighters, src/Gameplay hooks, tools/matchup_sim

## Technical references
Roadmap [24](../24_PRODUCTION_ROADMAP.md), architecture [18](../18_TECHNICAL_ARCHITECTURE.md), plus phase-relevant docs in [index](../README.md).

## Estimate and risk
6-10 FT weeks; see [26](../26_RISK_REGISTER.md).

## Tests
Automated: move tests, hook tests, matchup sim. Manual: 2 playtesters, 3 sessions. Build: Release build with 0 warnings, all CTest suites green on Linux (and Windows/WASM from P2).

## Acceptance checklist
- [ ] all moves reachable; 40-60% win rate band
- [ ] No unverified claim recorded as verified; evidence files saved under `docs/rebirth/evidence/`
- [ ] Docs updated if decisions changed

## Exit criteria / gate
Gate G4. Git checkpoint: tag `rebirth-phase-P4-done` only after the exit gate passes.

## Rollback / recovery
Revert data patches; never force-push; keep previous tags.

## Next phase dependencies
P5, P10
