# Recovery Run Evidence - 2026-11-25

| Field | Value |
|---|---|
| Purpose | Record the work and checks performed while resuming Ember Clash Rebirth from the supplied recovery archive. |
| Status | PARTIALLY VERIFIED - local targeted checks passed; full SDL build and CTest blocked by dependency network access. |
| Owner | Project owner (omarbt2); recovery changes prepared by ChatGPT |
| Depends on | 00_BASELINE_AUDIT.md, 00_SHARED_CONSTANTS.md, 14_INPUT_AND_ACCESSIBILITY.md |
| Related | P0_Recovery_and_Audit.md, 22_CI_AND_BUILD_MATRIX.md |
| Revision date | 2026-11-25 |
| Open questions | P1 design decisions remain open in 27_DECISION_LOG.md; full build/CTest must be run by CI or a machine with SDL3 fetch access. |

Evidence tags: VERIFIED, PARTIALLY VERIFIED, UNVERIFIED, MISSING, BROKEN. This record distinguishes the checks completed in this execution from historical evidence in the supplied bundle.

---

## 1. Source recovery

- The outer ZIP was opened and its nested source archive, Git bundles, production-document archive, and recovery notes were inventoried.
- The source bundle resolves to `dev/rebirth-continue` at `2eeceaea2452164e4fe8c064d8e0572ce6678b5e`. This is the source baseline used for the local recovery work.
- The production-document bundle resolves to `docs/rebirth-production-bible` at `d48fe96751594295f952c567c32353963ed2c4e6`, whose parent includes the source baseline. Its 55 Markdown documents, plus this new recovery evidence document, were checked with `tools/check_rebirth_docs.py` (56 files total).
- GitHub `master` is at `365348a2dacea4e03362bfd1fd718582361ba693`; it does not contain the recovered Rebirth source commit. The recovered source SHA is not present on GitHub, so a GitHub branch cannot be created directly from that SHA without uploading its Git objects first.

## 2. Changes prepared after the source baseline

- Corrected `InputManager::any_held()` to report the current key level across frames rather than a key-down event observed only in the current update.
- Added a non-aliasing invalid-key sentinel and rejected unsupported/unknown keycodes before indexing the key-state arrays.
- Sanitized malformed persisted key bindings per action, restoring the corresponding defaults while preserving valid SDL scancode-masked keys and intentionally unbound zero values.
- Expanded Settings from 7 to all 13 actions for both players, including the UI row map, key-name rendering, capture slot limits, and scrollable list rendering.
- Made the Controls page reflect the actual saved bindings instead of fixed defaults.
- Allowed Shift to be rebound for the SMASH MODIFIER action while continuing to reject other modifier-only bindings for ordinary actions.
- Re-applied live bindings after settings save and Restore Defaults, and refreshed the menu after a captured rebind.
- Removed the keyboard pause/Dodge collision: `P` remains Dodge; Escape pauses during play and backs out of menus; Gamepad Start pauses. Resuming with Escape/Start no longer immediately re-pauses, and Start resumes directly instead of confirming a selected pause-menu row.
- Rejected non-printable ASCII/C1 control code values as bindings.
- Updated regression assertions for the 39-row Settings page and the supported keycode rules.

## 3. Checks completed in this execution

| Check | Result | Scope / limitation |
|---|---|---|
| `git diff --check` | PASS | No whitespace errors in the recovery diff. |
| Documentation consistency checker | PASS | 56 Markdown documents; links, required headers, shared constants, phase dependencies, and estimate arithmetic. |
| Isolated `InputManager.cpp` + `Settings.cpp` compile | PASS | GCC 14.2, C++20, `-Wall -Wextra -Werror`, against a small local SDL API shim; not a substitute for the real SDL3 build. |
| Input/settings runtime probe | PASS | Persistent held-state, keycode uniqueness/validation, invalid-value fallback, arrow/keypad persistence, and the Escape/P key separation. |
| `Menu.cpp` syntax compile | PASS | GCC 14.2, C++20, `-Wall -Wextra -Werror`, against the same local SDL API shim. |
| Menu/settings runtime probe | PASS | 39 settings rows, 26 control bindings, correct P2 indices, dynamic Controls-page values. |
| Pause/resume runtime probe | PASS | Escape resumes with one resume action; Gamepad Start resumes directly instead of confirming the selected row. |
| `ember_test_m12.cpp` syntax compile | PASS | Test source parses with GCC 14.2 against the local SDL shim; the test executable was not linked or run. |
| Full CMake configure | BLOCKED | CMake FetchContent could not resolve `github.com` to download the pinned SDL3 `release-3.2.10` source. No game translation units were compiled by this configure attempt. |
| Full CTest suite | NOT RUN | The full build could not be configured without SDL3. |
| Desktop visual/audio/gamepad verification | NOT RUN | Requires the real SDL3 runtime and a display/audio/gamepad environment. |
| WebAssembly | PARTIALLY VERIFIED | The pre-existing committed `.wasm` module was accepted by Node's WebAssembly compiler; this is not a rebuild of the current C++ source. |

The SDL shim used for isolated checks exists only under `/tmp/fakesdl`; it is not part of the repository and does not replace SDL3.

## 4. Remaining production work

This recovery run is an input/settings correction, not completion of the Rebirth production plan. The plan still has open owner decisions and substantial missing work: movement/physics proof obligations, deeper combat mechanics, full fighter roster, authored art and animation, multiple complete arenas, training-grade AI, local multiplayer, modes/character-stage selection, online services, security, and release verification. Their scope and dependencies remain in `24_PRODUCTION_ROADMAP.md` and `work-packages/`.

The Linux GCC Release + headless CTest and Windows MSVC Release + CTest jobs are now defined in `.github/workflows/ci.yml`; it is **configured but not yet run**. Its first successful run is required before claiming a fresh CI green result.
