# Shared Constants (single source of truth)

| Field | Value |
|---|---|
| Purpose | The only place where cross-document numbers and names are defined. Every other document references these IDs. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | - |
| Related | all |
| Revision date | 2026-11-26 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

Changing a value here requires updating `tools/check_rebirth_docs.py` expectations and a Decision Log entry.

| ID | Constant | Value | Status |
|---|---|---|---|
| SC-1 | Simulation rate | 60 ticks/s; 1 tick = 1 frame = 1/60 s; gameplay timers are integer ticks | 60 Hz accumulator and dt-free simulation APIs implemented in local continuation work; integer-tick respawn/invulnerability/KO-banner timers implemented; D-001 still PROPOSED pending replay gate + owner approval |
| SC-2 | Max fighters per match | 4 (`kMaxFighters`); code today: `Match::kFighterCount = 2` | PROPOSED |
| SC-3 | Launch roster target / first-release roster | 8 fighters (long-term) / 4 fighters (first release, see 25) | PROPOSED |
| SC-4 | Arena target / first-release arenas | 6 / 3 | PROPOSED |
| SC-5 | AI difficulties | EASY, NORMAL, HARD, EXPERT | PROPOSED (code has kEasy, kStandard, kHard) |
| SC-6 | Input actions | 13: move_left, move_right, jump, fast_fall, up, jab, ftilt, nair, special, smash, shield, dodge, surge | VERIFIED in `InputMap.h` |
| SC-7 | Animation clips per fighter | 47 (list in 10_ANIMATION_AND_VFX.md) | PROPOSED |
| SC-8 | Logical render size | 1280x720 (`kRenderWidth` x `kRenderHeight`; letterboxed) | VERIFIED width; height per `Config.h` |
| SC-9 | Default stocks | 3 (`kStartingStocks`) | VERIFIED |
| SC-10 | Phase IDs | P0-P14 as in 24_PRODUCTION_ROADMAP.md | CONFIRMED structure |
| SC-11 | Release targets | Windows x64, Linux x64, WebAssembly | PROPOSED; none beyond Linux native verified |
| SC-12 | Content file format | JSON, schema in 19_DATA_AND_CONTENT_SCHEMA.md | PROPOSED |
| SC-13 | Reference hardware (target) | 4-core 2018-era CPU (e.g. i5-8400 class), integrated GPU (Intel UHD 630 class), 8 GB RAM | PROPOSED; no measurements yet |
| SC-14 | Frame budget | 16.67 ms total; sim tick <= 1.5 ms (4 fighters); render <= 8 ms | PROPOSED targets, unmeasured |
| SC-15 | Rollback window (online) | max 8 frames, input delay 2 frames default | PROPOSED, gate in 16 |
| SC-16 | Ranked eligibility | Ranked online is NOT in the first release; see 16 and 25 | PROPOSED |
