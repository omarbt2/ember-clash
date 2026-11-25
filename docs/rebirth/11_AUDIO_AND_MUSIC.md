# Audio and Music

| Field | Value |
|---|---|
| Purpose | Sound identity, assets and staged plan. |
| Status | DRAFT - pending owner review |
| Owner | Project owner (omarbt2); drafted by Claude |
| Depends on | 00_SHARED_CONSTANTS.md |
| Related | 09, 20, 28 |
| Revision date | 2026-11-24 |
| Open questions | See 27_DECISION_LOG.md |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. Shared constants are defined only in [00_SHARED_CONSTANTS.md](00_SHARED_CONSTANTS.md) and referenced as [SC-n].

---

Baseline: procedural synth in `Audio.cpp` (VERIFIED), tests pass headless; audible playback UNVERIFIED. This is prototype audio, not authored production audio.

## 1. Identity
Forge percussion, bowed metal, low brass; each fighter has a timbre (Kaelen metal ring, Torga stone/low drum, Vesper high chimes, Isolde glass/choir whisper, Rook mechanical clicks, Maren water/low harp, Zephyr wind/flute, Bastian bell/shield clang).

## 2. Categories
Impact: light/medium/heavy/KO; attacks: whoosh by weight; movement: jump/land/dash/ledge; UI: move/confirm/back/error; match: countdown, KO, victory, defeat; ambience per arena; music: menu, 3 launch arena themes (+3 post-launch), results sting.

## 3. Music structure
Loops of 60-90 s with 8-bar intro, stem layers (base + intensity) crossfaded at 50% damage/Surge; loop points as metadata.

## 4. Mixing and settings
Buses: Master, Music, SFX, UI; separate sliders (default 80/60/80/80); SFX priority limiter (max 24 voices); ducking of music under KO sting.

## 5. Spatial
Stereo pan by fighter x-position (+-50%), no HRTF.

## 6. Formats and licensing
OGG Vorbis 44.1 kHz (music stereo 128-160 kbps, SFX mono); memory budget in 20; all sounds original or licensed with record in 28.

## 7. Staged plan
S0 prototype synth (exists) -> S1 authored SFX set for 2 fighters + UI (P5) -> S2 first menu + arena 1 music (P7) -> S3 full launch set (P9-P10) -> S4 mastering and loudness (-14 LUFS music, -10 LUFS peak SFX) in P14. Loader and bus code needs SDL3 audio streams (SDL_mixer-free decision: D-020 open).

## 8. Validation
Automated: file format, length, loudness, loop points; Manual (real device only): playback, mix, clipping. Never claimed verified headless.
