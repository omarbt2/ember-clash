# Platform and Release

| Field | Value |
|---|---|
| Purpose | Targets and release process. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 22, 25 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

Essential for first release: Windows x64 zip/installer, Linux x64 tar.gz (AppImage optional), WebAssembly demo page; keyboard + gamepad; logical 1280x720 letterbox scaling, fullscreen toggle; settings/save in `SDL_GetPrefPath`; log file rotation (3 x 1 MB); crash reports: local minidump/log with opt-in upload (post-beta); semantic versioning `MAJOR.MINOR.PATCH+content_hash`; release candidates tagged `rcN`; demo build = 2 fighters, 1 arena; store prep (Steam page assets, trailer) in P14; closed playtests (10-20 testers) with feedback form; known-issue tracker in repo issues.
Optional: macOS, Steam Deck verification, controller glyph packs, achievements.
