# Release Scope and Milestones

| Field | Value |
|---|---|
| Purpose | Scope hierarchy and MoSCoW split. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 24, 16 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Milestones
| Milestone | Content | Phases |
|---|---|---|
| Technical prototype | existing baseline (2 AI/human fighters, one shared character) | P0 (done) |
| Combat prototype | deterministic movement + combat engine | P2-P3 |
| First playable vertical slice | 2 fighters, 1 arena with real art, AI vs human, local 2P | P4-P5 (+P7 first arena) |
| Local multiplayer alpha | 4 players, modes basic | P8 |
| Content-complete beta | 4 fighters, 3 arenas, modes, full UI, audio | P9, P10 wave A |
| Release candidate | polish, perf, platforms | P14 |
| First public release | see 2 | |
| Post-launch | fighters 5-8, arenas 4-6, online unranked, ranked | P10-P13 |

## 2. Minimum polished first release (Must)
4 fighters (Kaelen, Torga, Vesper, Isolde), 3 arenas (competitive set), AI 4 levels, Training, Versus AI, Local Versus, FFA, Team 2v2, Arcade; real art/animation/audio; menus; Windows/Linux (+ web demo); settings/rebinding/accessibility; save data.
## 3. MoSCoW
| Level | Items |
|---|---|
| Must | above; deterministic sim; 60 fps reference HW; crash-free 1000-match fuzz |
| Should | Survival; profile stats; replays (local); web build of full game |
| Could | fighters 5-8, arenas 4-6 (pull forward if capacity allows), cosmetics |
| Post-launch | online unranked 1v1 (P12), ranked (P13), spectators, reconnect, 2v2 online |
## 4. Online ranked decision
Ranked is delayed until local combat quality is proven in public play, the sim is deterministic across platforms, and a result-validation model of at least type C+D (16 section 7) is built and tested. Unranked online 1v1 is the first online step (D-014).
