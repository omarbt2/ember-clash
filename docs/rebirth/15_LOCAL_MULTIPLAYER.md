# Local Multiplayer

| Field | Value |
|---|---|
| Purpose | Design for up to four local players. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 14, 13, 18 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

Baseline: Match supports 2 fighters (`kFighterCount = 2`, VERIFIED); two human inputs tested at Match level only; menu has no local 2P entry (VERIFIED absence).
## 1. Requirements
Up to 4 players [SC-2]; join/leave, device assignment, keyboard profile handling (max 2 keyboard profiles, recommended 1), gamepad identification, hot-plug, character/team selection, FFA, 2v2, humans+AI, pause, rematch, disconnect handling.
## 2. Flow
Mode select -> Player Join (press button) -> Character select with per-slot cursor -> Team select (team modes) -> Stage -> Settings -> Match. AI slots added with the "+AI" button; difficulty per AI slot.
## 3. Rules
FFA: stocks each; ties via sudden death. Team: shared win condition, friendly fire setting default OFF, team tint outline. Pause: Start by any human pauses; only that player can resume; settings accessible.
## 4. Device loss
Gamepad disconnect: pause, show slot banner, 30 s to reconnect, else AI takes over or forfeits (setting).
## 5. Technical
`MatchConfig{slots[4]{device, fighter_id, team, ai_level}}`; Match iterates N fighters; per-slot `InputFrame` (D6) so the same code path serves online and AI.
## 6. Tests
Automated: 4-slot scripted match; input separation; slot hot-unplug via virtual device; deterministic 4-slot replay. Manual: 4 people on pads session; keyboard+3 pads; evidence = checklist with date and devices. Gamepad support is NOT claimed until the session passes.
