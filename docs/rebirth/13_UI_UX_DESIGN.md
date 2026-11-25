# UI/UX Design

| Field | Value |
|---|---|
| Purpose | Screens, flow, accessibility. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 08, 14, 15 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

Baseline (VERIFIED): menus PLAY / SETTINGS / CONTROLS / QUIT; HUD with damage %, stocks; KO banner; intro banners. No character/stage/mode select, no pause details beyond existing (UNVERIFIED), no results screen details.

## 1. Flow (text diagram)
```
Startup -> Main Menu -> Mode Select -> [Player Join] -> Character Select -> (Team Select) -> Stage Select -> Match Settings -> Loading -> Match(HUD) <-> Pause -> Results -> Rematch/Character Select/Main Menu
Main Menu -> Settings -> {Video, Audio, Controls, Accessibility}
Main Menu -> Training Setup -> Match (training HUD)
Main Menu -> Online (post-P12) -> Lobby/Private Room/Quick Match -> Character Select ...
Main Menu -> Profile (P9: local stats; P13: online profile)
```
Rule: a menu item appears only after the underlying feature is implemented and tested (SC-16/mode gating).

## 2. Screen table
| Screen | Purpose / layout | Navigation (kbd / pad) | States, errors, loading | Accessibility |
|---|---|---|---|---|
| Startup | logo, load assets, version | any key skips | loading bar; failure shows log path | no flashing > 3 Hz |
| Main menu | vertical list left, rotating fighter art right | W/S + Enter / D-pad + A | missing assets -> fallback + warning | text scaling 100-150% |
| Mode select | cards | arrows / D-pad | disabled cards hidden, not greyed | |
| Player join | 4 slots, "press any button" | per-device | device lost -> slot empty | icons+text |
| Character select | grid of 4-8, preview, color choice | per-player cursors | duplicate allowed in casual | colourblind tint |
| Team select | R/B/G/Y team column | | | |
| Stage select | thumbnails, ranked set flagged | | hazard toggle | |
| Match settings | stocks, timer, damage ratio | | | |
| HUD | %, stocks, Surge meter, name tags 1280x720 safe area 5% | - | | HUD scale option |
| Pause | resume, restart, settings, quit | Start/Esc | only pausing player may unpause (local) | |
| Results | winner, stats, rematch | | | |
| Settings/Controls/Controller config | rebinding, test input | | conflict warning (14) | |
| Training | overlay panel, reset hotkeys | | | |
| Online lobby/Private room/Profile/Ranked | later phases | | connection error taxonomy (16) | |

## 3. Responsive layout
Logical 1280x720 letterboxed; UI anchored to safe areas; supports 4:3 to 21:9 by letterbox (no stretch).

## 4. Visual identity
See 08 section 8.
