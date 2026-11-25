# Input and Accessibility

| Field | Value |
|---|---|
| Purpose | Input profiles, rebinding, buffering, accessibility. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 13, 15 |
| Revision date | 2026-11-25 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

## 1. Baseline (VERIFIED)
13 actions [SC-6] mapped in `KeyBindings` (A/D/W/S/J/K/L/I/U/LShift/O/P/Y for P1); keycode-aliasing bug fixed in `2eeceae`; two-human input tested at Match level; P2 keyboard bindings exist and the recovery UI now exposes all 13 actions for both profiles; P2 local-human play is still not wired in the app menu. Gamepad code exists in `Input/Gamepad.h` and `Haptics` - fighter control via pad is UNVERIFIED.

## 2. Profiles
Keyboard profile A (default P1), profile B (alternate P2 for local 1v1 only), up to 3 more players only by gamepad. Gamepad default: left stick/D-pad move, A jump, X jab, Y ftilt, B special, RB smash modifier, LB shield, RT dodge, LT/Back Surge (initial).
## 3. Assignment and hot-plug
Slots claimed by first button press; unplug during match pauses with "Reconnect or press any key to give control to AI"; SDL gamepad add/remove events handled on the main thread; IDs map to slots persistently per session.
## 4. Rebinding
Per profile; conflict detection with swap/cancel; reserved: Esc/Start for pause. Saved in settings file (versioned).
## 5. Buffering and priority
Buffer window 6 ticks before actionable; priority: shield > dodge > grab > special > smash > aerial > tilt > jab (initial); same-tick conflicts resolved by this order (deterministic).
## 6. Menu navigation
All menus operable with D-pad/stick/Enter/Esc; repeat delay 12 ticks.
## 7. Accessibility
Colour-blind palettes (3), team outlines with shapes, text scale 100/125/150%, reduced screen shake (0-100%), flash reduction, subtitles/visual cues for key audio (KO, Surge), one-handed alternate profile, audio bus sliders, hold-vs-toggle for charge actions.
## 8. Tests
Rebind conflict unit tests, slot isolation tests, buffer-window frame tests, hot-plug simulation using SDL virtual gamepads where available; hardware session checklist (manual).
