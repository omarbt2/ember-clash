# Post-Launch and Live Operations

| Field | Value |
|---|---|
| Purpose | Operations plan after first release. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 17, 25 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

Patches: monthly balance/bug patch cadence, versioned balance data (`content_hash`) and patch notes. Bug reporting: in-game "report" opens issue template with log; crash diagnostics via opt-in logs. Matchmaking/server monitoring and leaderboard integrity apply only after online release (17). Support: account recovery via Auth, abuse reports queue. Content: new fighters every 3-4 months, arenas every 6, each through the same phase gates (spec, tests, art validation). Cosmetics are optional and never affect gameplay; monetization design starts only after core fun is validated by playtests. Maintenance: dependency updates quarterly (SDL3), CI kept green, determinism job per release.
