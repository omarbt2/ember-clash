# Asset Inventory and Licenses

| Field | Value |
|---|---|
| Purpose | Tracking system for all assets. No authored assets exist today. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 09, 11 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Record fields
`id` (e.g. `FGT-KAE-ATLAS-001`), description, type (sprite/atlas/audio/music/font/ui/data), associated fighter/arena, source (concept path, tool+version, prompt log), license (own / CC0 / CC-BY / commercial licence ref), author, file path, status (planned/in-progress/done), validation status, integration status. Stored as `assets/INVENTORY.csv`; `LICENSES.md` summarizes third-party items; CI blocks release builds when a shipped file lacks a row.
## 2. Current inventory (audited)
| Category | State |
|---|---|
| Existing authored assets | none (no images/audio files in repo; `git ls-files` shows only code/docs and prebuilt `web/*`) |
| Temporary assets | procedural sprites/audio generated in code (not shippable art) |
| Generated assets | none (no generator was available) |
| Third-party | SDL3 (zlib licence) fetched at build time |
| Fonts | none selected; choose SIL OFL |
## 3. Rules
No asset ships without source+license; AI-generated assets require tool terms allowing commercial use and a logged prompt; no third-party IP; temporary assets marked `temp` and blocked from release builds.
