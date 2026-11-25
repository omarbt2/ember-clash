# Decision Log

| Field | Value |
|---|---|
| Purpose | All important decisions. Status: CONFIRMED / PROPOSED / OPEN / DEFERRED. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 00_PLAN_REVIEW_CORRECTIONS.md |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

| ID | Title | Status | Context | Options | Recommendation | Reasoning / consequences | Evidence required | Owner approval |
|---|---|---|---|---|---|---|---|---|
| D-000 | Keep C++20 + SDL3 + CMake | CONFIRMED (owner prompt) | existing codebase | engine switch | keep | working build, tests | - | confirmed |
| D-001 | Integer-tick time model, no dt in sim | PROPOSED | C1 | keep dt+assert | adopt with replay proof | needed for rollback | replay equivalence | Y |
| D-002 | Fixed-point Q16.16 vs constrained float | OPEN (gate) | C10 | Q16.16, Q24.8, float | run gate in P2; default Q16.16 if gate fails | determinism | cross-platform replay hashes | Y |
| D-003 | State snapshot POD design | PROPOSED | 18 | OO copy | POD fixed arrays | cheap rollback | completeness test | Y |
| D-004 | Art production method | OPEN | 09 | A/B/C | spike; working hypothesis A | consistency vs look | spike go/no-go | Y |
| D-005 | Roster names/concepts | CONFIRMED provisional (owner) | prompt | edit | keep | - | - | Y for changes |
| D-006 | First-release roster = 4 | PROPOSED | C7 | 2, 4, 8 | 4 | schedule | spike metrics | Y |
| D-007 | First-release arenas = 3 competitive | PROPOSED | 07 | 1-6 | 3 | schedule | - | Y |
| D-008 | Grabs/throws included | PROPOSED | C14 | no grabs | include | shield counter | playtest | Y |
| D-009 | AI reaction bands | PROPOSED | C2 | 6-8 f | 12-16 EXPERT floor | fairness | playtest | Y |
| D-010 | Data-driven JSON content + C++ hooks | PROPOSED | 19 | scripting | JSON+hooks | simple | schema tests | Y |
| D-011 | Staleness mechanic | PROPOSED | 03 | none | include light | anti-spam | playtest | N |
| D-012 | Backend: Supabase + own relay | PROPOSED | 17 | Nakama/PlayFab | Supabase+relay | owner stack | prototype | Y |
| D-013 | Native vs web netplay interop | OPEN | 16 | separate pools / WebRTC both | separate pools first | complexity | prototype | Y |
| D-014 | Ranked after first release | PROPOSED | C3 | ship ranked | defer | integrity | model C+D built | Y |
| D-015 | CI on GitHub Actions | PROPOSED | 22 | other | GH Actions | repo on GitHub | CI run | N |
| D-016 | Release scope staging | PROPOSED | C5 | full scope | staged | schedule | - | Y |
| D-017 | Audio runtime (SDL3 streams vs library) | OPEN | 11 | SDL_mixer, miniaudio | SDL3 streams | fewer deps | prototype | N |
| D-018 | Monetization: cosmetics only | CONFIRMED principle (owner) | prompt | - | no P2W | - | - | confirmed |
| D-019 | No push to GitHub in this task | CONFIRMED (owner) | - | - | - | - | - | - |
| D-020 | Git LFS for art | PROPOSED | 09 | external storage | LFS | simple | quota check | N |
