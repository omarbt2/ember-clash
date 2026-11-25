#include "UI/Menu.h"

#include "Audio/Audio.h"
#include "Core/Config.h"
#include "Core/Logger.h"
#include "Input/Gamepad.h"
#include "Input/InputManager.h"
#include "UI/BitmapFont.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_keycode.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

namespace ember {

namespace {

// Render-space geometry for the menu UI. The logical render is
// 1280x720 (see Config.h). Menu elements snap to the centre line
// and to fixed margins so the layout works on every aspect ratio
// the window can be resized to.
constexpr int kScreenW = 1280;
constexpr int kScreenH = 720;

constexpr int kTitleY        = 200;
constexpr int kMenuTopY      = 320;
constexpr int kMenuLineDy    = 48;
constexpr int kMenuLeftX     = 380;
constexpr int kHintY         = 640;
constexpr int kQuitConfirmY  = 360;
constexpr int kSettingLeftX  = 240;
constexpr int kSettingValX   = 920;

constexpr config::Color kFocusColor  { 250, 220,  80, 255 };
constexpr config::Color kTitleColor  { 240, 200,  80, 255 };
constexpr config::Color kHintColor   { 180, 180, 200, 255 };
constexpr config::Color kBgColor     {  18,  22,  32, 200 };  // translucent overlay
constexpr config::Color kPanelColor  {  40,  44,  60, 220 };

// Format an integer percentage (0-100) into a right-aligned string.
std::string fmt_pct(int v) {
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    return std::to_string(v);
}

// Format a bool as "ON" / "OFF".
std::string fmt_onoff(bool v) { return v ? "ON" : "OFF"; }

std::string fmt_key_label(int keycode) {
    if (!InputManager::is_valid_keycode(static_cast<SDL_Keycode>(keycode))) return "---";
    const char* raw = SDL_GetKeyName(static_cast<SDL_Keycode>(keycode));
    if (!raw || !*raw) return "KEY";
    std::string result(raw);
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return result;
}

// Center a string of N characters horizontally on a row at y. The
// bitmap font is 5 cells per glyph and a 4-wide glyph, so the
// visible width is N * 5 - 1 cells. (see measure_text)
void draw_text_centered(Renderer& r, int y, const std::string& s,
                        config::Color color) {
    const int w = ui::measure_text(s.c_str());
    const int x = (kScreenW - w) / 2;
    ui::draw_text(r, x, y, s.c_str(), color);
}

// Draw a left-aligned entry at row y and (optionally) a right-
// aligned value at x_value. The label uses the focus color when
// this is the selected row, otherwise the standard HUD text color.
void draw_entry(Renderer& r, int y, const Menu::Entry& e, bool focused,
                int value_x) {
    const config::Color label_col = focused ? kFocusColor
                                            : config::kHudTextColor;
    ui::draw_text(r, kMenuLeftX, y, e.label.c_str(), label_col);
    if (!e.value.empty()) {
        const int w = ui::measure_text(e.value.c_str());
        ui::draw_text(r, value_x - w, y, e.value.c_str(), label_col);
    }
    // Selection cursor: a small filled square to the left of the
    // label. Avoids relying on a font glyph (the ">" character
    // would be hard to spot at this size).
    if (focused) {
        r.draw_filled_rect({static_cast<float>(kMenuLeftX - 24),
                            static_cast<float>(y + 6), 8.0f, 8.0f},
                           kFocusColor);
    }
}

// One-time construction of the static entry lists. Called from
// the Menu constructor.
Menu::Entry mk(const char* l, Menu::Action a) {
    Menu::Entry e;
    e.label = l;
    e.value.clear();
    e.on_confirm = a;
    return e;
}
Menu::Entry mkv(const char* l, const char* v, Menu::Action a) {
    Menu::Entry e;
    e.label = l;
    e.value = v;
    e.on_confirm = a;
    return e;
}

} // namespace

Menu::Menu() {
    main_entries_ = {
        mk ("PLAY",        Action::kStartMatch),
        mk ("SETTINGS",    Action::kSaveSettings),  // "open" via state machine, not action
        mk ("CONTROLS",    Action::kSaveSettings),  // ditto
        mk ("QUIT",        Action::kRequestQuit),
    };
    mode_entries_ = {
        mk ("VERSUS CPU",      Action::kStartMatch),
        mk ("LOCAL 2 PLAYERS", Action::kStartLocalTwoPlayer),
        mk ("FREE TRAINING",   Action::kStartTraining),
        mk ("BACK",            Action::kNone),
    };
    arena_entries_ = {
        mk ("TRAINING RING",   Action::kStartMatch),
        mk ("EMBER FOUNDRY",   Action::kStartMatch),
        mk ("SKY TEMPLE",      Action::kStartMatch),
        mk ("BACK",            Action::kNone),
    };
    // We can't pass kOpenSettings / kOpenControls as Actions
    // because the Action enum is the OUT-pipe from the menu. The
    // trick: the menu's update() recognises the selected entry
    // and switches state directly when confirm fires on these
    // rows. See navigate() for the dispatch.

    paused_entries_ = {
        mk ("RESUME",       Action::kResumeMatch),
        mk ("SETTINGS",     Action::kSaveSettings),
        mk ("CONTROLS",     Action::kSaveSettings),
        mk ("RESTART",      Action::kRestartMatch),
        mk ("QUIT TO TITLE",Action::kReturnToTitle),
    };

    rematch_entries_ = {
        mk ("REMATCH",      Action::kRestartMatch),
        mk ("BACK TO TITLE",Action::kReturnToTitle),
    };

    rebuild_settings_entries();
}

void Menu::rebuild_settings_entries() {
    settings_entries_.clear();
    if (settings_) {
        settings_entries_.push_back(mkv("MASTER VOLUME",
            (fmt_pct(settings_->master_volume) + "%").c_str(),
            Action::kNone));
        settings_entries_.push_back(mkv("MASTER MUTE",
            fmt_onoff(settings_->master_mute).c_str(),
            Action::kNone));
        settings_entries_.push_back(mkv("SFX VOLUME",
            (fmt_pct(settings_->sfx_volume) + "%").c_str(),
            Action::kNone));
        settings_entries_.push_back(mkv("SFX MUTE",
            fmt_onoff(settings_->sfx_mute).c_str(),
            Action::kNone));
        settings_entries_.push_back(mkv("MUSIC VOLUME",
            (fmt_pct(settings_->music_volume) + "%").c_str(),
            Action::kNone));
        settings_entries_.push_back(mkv("MUSIC MUTE",
            fmt_onoff(settings_->music_mute).c_str(),
            Action::kNone));
        settings_entries_.push_back(mkv("FULLSCREEN",
            fmt_onoff(settings_->fullscreen).c_str(),
            Action::kToggleFullscreen));
        settings_entries_.push_back(mkv("SCREEN SHAKE",
            fmt_onoff(settings_->screen_shake).c_str(),
            Action::kNone));
        settings_entries_.push_back(mkv("COLOR BLIND",
            fmt_onoff(settings_->color_blind_palette).c_str(),
            Action::kNone));
        settings_entries_.push_back(mkv("AI DIFFICULTY",
            std::string(settings_->ai_difficulty.data()).c_str(),
            Action::kNone));
        settings_entries_.push_back(mkv("INTERNAL RES",
            (std::string("x") + std::to_string(settings_->render_scale)
                + "  (1x = 1280x720)").c_str(),
            Action::kApplyRenderScale));
        // M13 / M13.1: P1 and P2 rebindable keys. The label
        // and value are read from the live settings struct so
        // the user can see the active binding change as they
        // rebind. The confirm handler in navigate() translates
        // one of these indices into the remap-capture
        // sub-state. P2 defaults to the arrow-keys / numpad
        // layout, distinct from P1's WASD+JKL.
        for (int player = 0; player < 2; ++player) {
            // Take a pointer to the underlying C array. The
            // std::array decay to int* is non-const-only, so
            // we go through .data() to be explicit and const-
            // safe.
            const int* slot_array = (player == 0)
                ? settings_->p1_bindings.data()
                : settings_->p2_bindings.data();
            const char* pfx = (player == 0) ? "P1 " : "P2 ";
            static constexpr const char* kActionLabels[13] = {
                "MOVE LEFT", "MOVE RIGHT", "JUMP", "FAST FALL", "JAB",
                "FORWARD TILT", "NEUTRAL AIR", "AIM UP", "SPECIAL",
                "SMASH MODIFIER", "SHIELD", "DODGE", "EMBER SURGE"
            };
            for (int i = 0; i < kSettingsActionsPerPlayer; ++i) {
                char full_lbl[32];
                std::snprintf(full_lbl, sizeof(full_lbl), "%s%s", pfx, kActionLabels[i]);
                const std::string key_label = fmt_key_label(slot_array[i]);
                settings_entries_.push_back(mkv(full_lbl, key_label.c_str(), Action::kNone));
            }
        }
        settings_entries_.push_back(mk ("RESTORE DEFAULTS",
            Action::kRestoreDefaultSettings));
        settings_entries_.push_back(mk ("BACK",
            Action::kNone));
    } else {
        settings_entries_.push_back(mk("(no settings bound)", Action::kNone));
    }
    rebuild_controls_entries();
}

void Menu::rebuild_controls_entries() {
    static constexpr const char* kActionLabels[13] = {
        "MOVE LEFT", "MOVE RIGHT", "JUMP", "FAST FALL", "JAB",
        "FORWARD TILT", "NEUTRAL AIR", "AIM UP", "SPECIAL",
        "SMASH MODIFIER", "SHIELD", "DODGE", "EMBER SURGE"
    };

    Settings defaults;
    defaults.reset_to_defaults();
    const auto& p1 = settings_ ? settings_->p1_bindings : defaults.p1_bindings;
    const auto& p2 = settings_ ? settings_->p2_bindings : defaults.p2_bindings;

    controls_entries_.clear();
    controls_entries_.reserve(29);
    for (int i = 0; i < 13; ++i) {
        const std::string label = std::string("P1 ") + kActionLabels[i];
        const std::string key = fmt_key_label(p1[static_cast<std::size_t>(i)]);
        controls_entries_.push_back(mkv(label.c_str(), key.c_str(), Action::kNone));
    }
    for (int i = 0; i < 13; ++i) {
        const std::string label = std::string("P2 ") + kActionLabels[i];
        const std::string key = fmt_key_label(p2[static_cast<std::size_t>(i)]);
        controls_entries_.push_back(mkv(label.c_str(), key.c_str(), Action::kNone));
    }
    controls_entries_.push_back(mkv("PAUSE", "ESC / GAMEPAD START", Action::kNone));
    controls_entries_.push_back(mkv("CONFIRM (MENU)", "ENTER / GAMEPAD A", Action::kNone));
    controls_entries_.push_back(mkv("CANCEL (MENU)", "ESC / GAMEPAD B", Action::kNone));
    controls_entries_.push_back(mkv("BACK TO TITLE", "BACK (MENU)", Action::kReturnToTitle));
}

const std::vector<Menu::Entry>& Menu::entries_for_state(State s) const {
    switch (s) {
        case State::kMain:        return main_entries_;
        case State::kModeSelect:  return mode_entries_;
        case State::kArenaSelect: return arena_entries_;
        case State::kPaused:      return paused_entries_;
        case State::kRematch:     return rematch_entries_;
        case State::kSettings:    return settings_entries_;
        case State::kControls:    return controls_entries_;
        case State::kTitle:       return main_entries_;  // unused; title is bespoke
        case State::kPlay:        return main_entries_;  // unused; dormant
        case State::kQuitConfirm: return main_entries_;  // unused; bespoke
        case State::kRemapCapture:return main_entries_;  // unused; bespoke
    }
    return main_entries_;
}

void Menu::push_event(Event ev) {
    handle_event(ev);
}

bool Menu::handle_event(Event ev) {
    switch (ev) {
        case Event::kMatchStarted:
            state_ = State::kPlay;
            selected_index_ = 0;
            return true;
        case Event::kMatchEnded:
            state_ = State::kRematch;
            selected_index_ = 0;
            return true;
        case Event::kMatchPaused:
            if (state_ == State::kPlay) {
                state_ = State::kPaused;
                selected_index_ = 0;
            }
            return true;
        case Event::kMatchResumed:
            if (state_ == State::kPaused) {
                state_ = State::kPlay;
                selected_index_ = 0;
            }
            return true;
    }
    return false;
}

Menu::Action Menu::navigate(const InputManager& input, const Gamepad& pad,
                            Audio& audio) {
    auto& entries = entries_for_state(state_);
    if (entries.empty()) return Action::kNone;

    // Up / down cycle through the entry list. The "menu up" /
    // "menu down" predicates from InputManager are already edge-
    // triggered, so a single press advances one row.
    const bool up   = input.menu_up()    || pad.nav_up();
    const bool down = input.menu_down()  || pad.nav_down();
    if (up) {
        selected_index_ = (selected_index_ - 1 + static_cast<int>(entries.size()))
                          % static_cast<int>(entries.size());
        audio.play(SfxId::MenuMove);
    }
    if (down) {
        selected_index_ = (selected_index_ + 1) % static_cast<int>(entries.size());
        audio.play(SfxId::MenuMove);
    }

    // Cancel = back / leave the current sub-screen.
    if (input.menu_cancel() || pad.cancel()) {
        if (state_ == State::kModeSelect) {
            state_ = State::kMain;
            selected_index_ = 0;
            audio.play(SfxId::MenuCancel);
            return Action::kNone;
        }
        if (state_ == State::kArenaSelect) {
            state_ = State::kModeSelect;
            selected_index_ = 0;
            audio.play(SfxId::MenuCancel);
            return Action::kNone;
        }
        if (state_ == State::kSettings || state_ == State::kControls) {
            // Save settings on the way out of the settings screen.
            const bool leaving_settings = state_ == State::kSettings;
            if (leaving_settings) pending_action_ = Action::kSaveSettings;
            state_ = State::kMain;
            selected_index_ = leaving_settings ? 1 : 2;
            audio.play(SfxId::MenuCancel);
            return Action::kNone;
        }
        if (state_ == State::kPaused) {
            // Cancel from pause = unpause (standard "back" in pause
            // menus).
            audio.play(SfxId::MenuBack);
            handle_event(Event::kMatchResumed);
            return Action::kResumeMatch;
        }
        if (state_ == State::kQuitConfirm) {
            state_ = (state_ == State::kQuitConfirm) ? State::kTitle : State::kTitle;
            audio.play(SfxId::MenuCancel);
            return Action::kNone;
        }
    }

    // Confirm = execute the focused entry.
    if (input.menu_confirm() || pad.confirm()) {
        // The settings screen has its own dispatch (because its
        // entries edit values in place rather than emitting
        // actions). Other states branch on selected_index_
        // directly below; we intentionally do NOT read entries
        // here because the Entry's on_confirm Action is only
        // used by the bookkeeping calls in those branches.
        (void)entries;
        // Settings screen: the right-hand value column is editable
        // with left/right. Confirm is reserved for BIND rows (M13)
        // and for the BOOLEAN rows (mute / fullscreen / screen shake
        // / color blind) that have only an on/off state. The
        // CONTINUOUS rows (volume sliders, AI difficulty cycle,
        // render scale cycle) are still driven by confirm, with the
        // +/- flow on left/right reserved for slider-style fields.
        // P15: after every change we call audio.apply_settings()
        // and the App's "apply live" hook so the change is
        // observable to the user immediately. Without this, a
        // volume change would not affect the next play() until
        // the user backed out to the main menu, which made the
        // settings feel broken.
        if (state_ == State::kSettings && settings_) {
            const int bind_offset = selected_index_ - kSettingsBindStartIndex;
            if (bind_offset >= 0 &&
                bind_offset < 2 * kSettingsActionsPerPlayer) {
                remap_slot_ = bind_offset % kSettingsActionsPerPlayer;
                remap_player_ = bind_offset / kSettingsActionsPerPlayer;
                remap_return_index_ = selected_index_;
                remap_capture_active_ = true;
                remap_pulse_ticks_ = 0;
                state_ = State::kRemapCapture;
                audio.play(SfxId::MenuConfirm);
                return Action::kNone;
            }
            switch (selected_index_) {
                case 0:
                case 2:
                case 4:
                    // Volume rows: confirm is a no-op. The +/-10
                    // edit happens via left/right (see below).
                    // If the user has landed here by mistake we
                    // just play the "back" cue so they know
                    // nothing happened, and we do NOT change
                    // the value.
                    audio.play(SfxId::MenuBack);
                    rebuild_settings_entries();
                    return Action::kNone;
                case 1: settings_->master_mute = !settings_->master_mute; break;
                case 3: settings_->sfx_mute    = !settings_->sfx_mute;    break;
                case 5: settings_->music_mute  = !settings_->music_mute;  break;
                case 6: settings_->fullscreen = !settings_->fullscreen; break;
                case 7: settings_->screen_shake = !settings_->screen_shake; break;
                case 8: settings_->color_blind_palette = !settings_->color_blind_palette; break;
                case 9: {
                    // Cycle all shipped CPU tiers in ascending challenge.
                    const std::string cur = std::string(settings_->ai_difficulty.data());
                    std::string next;
                    if      (cur == "easy")     next = "standard";
                    else if (cur == "standard") next = "hard";
                    else if (cur == "hard")     next = "expert";
                    else                         next = "easy";
                    std::memset(settings_->ai_difficulty.data(), 0,
                                settings_->ai_difficulty.size());
                    std::memcpy(settings_->ai_difficulty.data(), next.data(),
                                std::min<std::size_t>(next.size(),
                                                      settings_->ai_difficulty.size() - 1));
                    break;
                }
                case 10: {
                    // Render scale: cycle 1 -> 2 -> 3 -> 4 -> 1.
                    settings_->render_scale = (settings_->render_scale % 4) + 1;
                    break;
                }
                case kSettingsRestoreDefaultsIndex: settings_->reset_to_defaults(); break;
                case kSettingsBackIndex: {
                    // BACK row
                    pending_action_ = Action::kSaveSettings;
                    state_ = State::kMain;
                    selected_index_ = 1;
                    audio.play(SfxId::MenuBack);
                    return Action::kNone;
                }
            }
            if (selected_index_ < kSettingsBackIndex) {
                audio.play(SfxId::MenuConfirm);
            }
            // P15: live-apply audio settings so a volume change
            // is audible the instant the slider moves. The
            // render-scale + fullscreen + color-blind changes
            // are observed by App via the SettingsChanged
            // action that kSaveSettings emits; we ALSO want
            // the audio apply to be live here, so we call it
            // directly rather than waiting for the BACK path.
            audio.apply_settings(*settings_);
            // P15: mark the file dirty so the periodic flush
            // (App::step, last_settings_flush_) writes the new
            // value to disk. Previously a change made in the
            // settings UI was lost on crash if the player
            // never pressed BACK.
            pending_action_ = Action::kSaveSettings;
            rebuild_settings_entries();
            return Action::kNone;
        }

        // Non-settings states: the entry's on_confirm Action
        // tells us what to do. A few entries (SETTINGS, CONTROLS,
        // QUIT) need a state transition, not an action - we
        // handle those explicitly.
        if (state_ == State::kMain) {
            switch (selected_index_) {
                case 0: // PLAY -> mode selection
                    audio.play(SfxId::MenuConfirm);
                    state_ = State::kModeSelect;
                    selected_index_ = 0;
                    return Action::kNone;
                case 1: // SETTINGS
                    audio.play(SfxId::MenuConfirm);
                    state_ = State::kSettings;
                    selected_index_ = 0;
                    return Action::kNone;
                case 2: // CONTROLS
                    audio.play(SfxId::MenuConfirm);
                    state_ = State::kControls;
                    selected_index_ = 0;
                    return Action::kNone;
                case 3: // QUIT
                    audio.play(SfxId::MenuConfirm);
                    state_ = State::kQuitConfirm;
                    selected_index_ = 0;
                    return Action::kNone;
            }
        }
        if (state_ == State::kModeSelect) {
            if (selected_index_ == 3) {
                state_ = State::kMain;
                selected_index_ = 0;
                audio.play(SfxId::MenuBack);
                return Action::kNone;
            }
            pending_mode_action_ = selected_index_ == 1 ? Action::kStartLocalTwoPlayer
                : selected_index_ == 2 ? Action::kStartTraining : Action::kStartMatch;
            state_ = State::kArenaSelect;
            selected_index_ = 0;
            audio.play(SfxId::MenuConfirm);
            return Action::kNone;
        }
        if (state_ == State::kArenaSelect) {
            if (selected_index_ == 3) {
                state_ = State::kModeSelect;
                selected_index_ = 0;
                audio.play(SfxId::MenuBack);
                return Action::kNone;
            }
            selected_arena_index_ = selected_index_;
            state_ = State::kPlay;
            audio.play(SfxId::MenuConfirm);
            return pending_mode_action_;
        }
        if (state_ == State::kPaused) {
            switch (selected_index_) {
                case 0: // RESUME
                    audio.play(SfxId::MenuConfirm);
                    handle_event(Event::kMatchResumed);
                    return Action::kResumeMatch;
                case 1: // SETTINGS
                    audio.play(SfxId::MenuConfirm);
                    state_ = State::kSettings;
                    selected_index_ = 0;
                    return Action::kNone;
                case 2: // CONTROLS
                    audio.play(SfxId::MenuConfirm);
                    state_ = State::kControls;
                    selected_index_ = 0;
                    return Action::kNone;
                case 3: // RESTART
                    audio.play(SfxId::MenuConfirm);
                    handle_event(Event::kMatchResumed);
                    return Action::kRestartMatch;
                case 4: // QUIT TO TITLE
                    audio.play(SfxId::MenuConfirm);
                    return Action::kReturnToTitle;
            }
        }
        if (state_ == State::kRematch) {
            switch (selected_index_) {
                case 0: // REMATCH
                    audio.play(SfxId::MenuConfirm);
                    return Action::kRestartMatch;
                case 1: // BACK TO TITLE
                    audio.play(SfxId::MenuConfirm);
                    return Action::kReturnToTitle;
            }
        }
        if (state_ == State::kQuitConfirm) {
            switch (selected_index_) {
                case 0: // YES
                    audio.play(SfxId::MenuConfirm);
                    return Action::kRequestQuit;
                case 1: // NO
                    audio.play(SfxId::MenuCancel);
                    state_ = State::kTitle;
                    return Action::kNone;
            }
        }
        if (state_ == State::kControls) {
            // The "BACK TO TITLE" row is the last entry.
            if (selected_index_ == static_cast<int>(entries.size()) - 1) {
                audio.play(SfxId::MenuBack);
                state_ = State::kMain;
                selected_index_ = 2;
                return Action::kNone;
            }
        }
    }

    // Left/right also works for +/- on volume entries. Treat the
    // current selection as the edit target; left decreases, right
    // increases. Mute / fullscreen / AI difficulty are handled by
    // confirm only - left/right on those rows is a no-op.
    // P15: the volume slider is now STRICTLY 0..100 (no wrap-
    // around at 100->0). Pressing right at 100 does nothing
    // and the slider emits the "back" cue; pressing left at
    // 0 does nothing. This matches the platform-fighter norm
    // and matches what the user expects from a volume slider.
    if (state_ == State::kSettings && settings_) {
        const bool left  = input.menu_left()  || pad.nav_left();
        const bool right = input.menu_right() || pad.nav_right();
        if (left || right) {
            const int delta = right ? +10 : -10;
            bool changed = false;
            switch (selected_index_) {
                case 0: {
                    int& v = settings_->master_volume;
                    const int next = v + delta;
                    if (next >= 0 && next <= 100) {
                        v = next;
                        changed = true;
                    }
                } break;
                case 2: {
                    int& v = settings_->sfx_volume;
                    const int next = v + delta;
                    if (next >= 0 && next <= 100) {
                        v = next;
                        changed = true;
                    }
                } break;
                case 4: {
                    int& v = settings_->music_volume;
                    const int next = v + delta;
                    if (next >= 0 && next <= 100) {
                        v = next;
                        changed = true;
                    }
                } break;
                default: break;
            }
            if (changed) {
                audio.play(SfxId::MenuMove);
                // P15: live-apply so the player hears the new
                // volume right away. Without this, dragging the
                // slider felt broken because the audio was
                // unchanged until the user backed out to the
                // main menu.
                audio.apply_settings(*settings_);
                pending_action_ = Action::kSaveSettings;
                rebuild_settings_entries();
            } else {
                // Slider pegged at one end - play a quieter
                // cue so the user knows the press was
                // registered but no change happened.
                audio.play(SfxId::MenuBack);
            }
        }
    }

    return Action::kNone;
}

Menu::State Menu::update(const InputManager& input, const Gamepad& pad,
                         Audio& audio) {
    ++title_pulse_ticks_;

    // The play state is dormant: input still flows to App (which
    // pumps it into Match), the menu just doesn't react.
    if (state_ == State::kPlay) {
        return state_;
    }

    if (state_ == State::kTitle) {
        // "Press any key to start" - any menu input advances to
        // the main menu.
        if (input.menu_any_pressed() || pad.confirm()) {
            state_ = State::kMain;
            selected_index_ = 0;
            audio.play(SfxId::MenuConfirm);
        }
        return state_;
    }

    // -----------------------------------------------------------------
    // M13 remap-capture sub-state. We block normal navigation
    // here; the only meaningful inputs are:
    //   - a new key-down: commit that keycode to the slot
    //     and emit kApplyRemap for App to flush to disk.
    //   - ESC: cancel and return to kSettings.
    //   - any other key: ignored. Gamepad input is also
    //     ignored so the player does not accidentally bind a
    //     stick direction to a fighter action (gamepad binds
    //     are a future feature).
    // -----------------------------------------------------------------
    // Start is a dedicated pause/resume control, not a generic confirm
    // button while the pause menu is open (where it could activate the
    // currently selected Settings/Restart/Quit row by accident).
    if (state_ == State::kPaused && pad.pressed(SDL_GAMEPAD_BUTTON_START)) {
        audio.play(SfxId::Pause);
        handle_event(Event::kMatchResumed);
        pending_action_ = Action::kResumeMatch;
        return state_;
    }

    if (state_ == State::kRemapCapture) {
        ++remap_pulse_ticks_;
        if (input.menu_cancel() || pad.cancel()) {
            // Cancel: leave the slot untouched and pop back
            // to the settings list. The player is left on the
            // same row they were editing.
            audio.play(SfxId::MenuCancel);
            remap_capture_active_ = false;
            remap_slot_ = -1;
            remap_pulse_ticks_ = 0;
            state_ = State::kSettings;
            rebuild_settings_entries();
            // Restore selection to the original bind row so
            // the cursor doesn't jump.
            selected_index_ = remap_return_index_;
            return state_;
        }
        const SDL_Keycode k = input.any_pressed_key();
        if (k != 0) {
            // Pure modifiers are rejected for ordinary actions because
            // they are difficult to use safely as a standalone button.
            // The SMASH MODIFIER action (slot 9) is the deliberate
            // exception: Shift is its natural binding and must be
            // rebindable just like every other action.
            const bool is_shift = k == SDLK_LSHIFT || k == SDLK_RSHIFT;
            const bool is_other_modifier =
                k == SDLK_LCTRL || k == SDLK_RCTRL ||
                k == SDLK_LALT  || k == SDLK_RALT  ||
                k == SDLK_LGUI  || k == SDLK_RGUI;
            if (!is_other_modifier && (!is_shift || remap_slot_ == 9)) {
                if (remap_slot_ >= 0 && remap_slot_ < kSettingsActionsPerPlayer) {
                    pending_remap_player_  = remap_player_;
                    pending_remap_slot_    = remap_slot_;
                    pending_remap_keycode_ = static_cast<int>(k);
                    pending_action_ = Action::kApplyRemap;
                }
                audio.play(SfxId::MenuConfirm);
            } else {
                // Modifier-only: treat as cancel-by-mistake.
                audio.play(SfxId::MenuBack);
            }
            remap_capture_active_ = false;
            remap_slot_ = -1;
            remap_player_ = 0;
            remap_pulse_ticks_ = 0;
            state_ = State::kSettings;
            rebuild_settings_entries();
            // App will refresh the values again after it has
            // committed the captured key into Settings.
            selected_index_ = remap_return_index_;
            return state_;
        }
        return state_;
    }

    const Action a = navigate(input, pad, audio);
    if (a != Action::kNone) {
        pending_action_ = a;
    }
    return state_;
}

Menu::Action Menu::consume_action() {
    const Action a = pending_action_;
    pending_action_ = Action::kNone;
    return a;
}

bool Menu::blocks_simulation() const {
    return state_ != State::kPlay;
}

// ---- Rendering ----------------------------------------------------------------

void Menu::render_title(Renderer& r) const {
    // Background.
    r.draw_filled_rect({0, 0, static_cast<float>(kScreenW),
                        static_cast<float>(kScreenH)}, kBgColor);
    // Big title.
    draw_text_centered(r, kTitleY, "EMBER CLASH", kTitleColor);

    // Pulse the "press any key" hint. The pulse is a 30-tick
    // triangle wave so the hint fades in and out smoothly.
    const int period = 60;
    const int phase  = title_pulse_ticks_ % period;
    const int bright = (phase < period / 2) ? phase : (period - phase);
    const config::Color hint_col {
        static_cast<std::uint8_t>(120 + bright),
        static_cast<std::uint8_t>(120 + bright),
        static_cast<std::uint8_t>(140 + bright),
        255 };
    draw_text_centered(r, kHintY, "PRESS ANY KEY", hint_col);

    // Subtitle. Shows keyboard + gamepad hints so the player
    // knows both devices are supported. We do not need the
    // gamepad name here (the controls screen has that).
    draw_text_centered(r, kHintY + 28,
                       "KEYBOARD OR GAMEPAD",
                       kHintColor);
}

void Menu::render_main(Renderer& r) const {
    // Translucent background panel.
    r.draw_filled_rect({0, 0, static_cast<float>(kScreenW),
                        static_cast<float>(kScreenH)}, kBgColor);
    draw_text_centered(r, kTitleY, "EMBER CLASH", kTitleColor);
    if (state_ == State::kModeSelect)
        draw_text_centered(r, 235, "CHOOSE A MODE", kHintColor);
    else if (state_ == State::kArenaSelect)
        draw_text_centered(r, 235, "CHOOSE AN ARENA", kHintColor);

    const auto& entries = entries_for_state(state_);
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const int y = kMenuTopY + static_cast<int>(i) * kMenuLineDy;
        draw_entry(r, y, entries[i],
                   static_cast<int>(i) == selected_index_,
                   kSettingValX);
    }
    draw_text_centered(r, kHintY, "UP/DOWN: SELECT   ENTER: CONFIRM   ESC: BACK",
                       kHintColor);
}

void Menu::render_paused(Renderer& r) const {
    // Draw the underlying match would be nice but we'd need the
    // match to render itself; for V1.0 we just dim the whole
    // screen with a darker panel. The match is still ticking
    // behind the menu because Match::update is the call site
    // owner (App stops calling it when the menu blocks sim).
    r.draw_filled_rect({0, 0, static_cast<float>(kScreenW),
                        static_cast<float>(kScreenH)}, kPanelColor);
    draw_text_centered(r, 200, "PAUSED", kTitleColor);

    const auto& entries = entries_for_state(state_);
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const int y = kMenuTopY + static_cast<int>(i) * kMenuLineDy;
        draw_entry(r, y, entries[i],
                   static_cast<int>(i) == selected_index_,
                   kSettingValX);
    }
    draw_text_centered(r, kHintY, "ESC / GAMEPAD B: RESUME", kHintColor);
}

void Menu::render_rematch(Renderer& r) const {
    r.draw_filled_rect({0, 0, static_cast<float>(kScreenW),
                        static_cast<float>(kScreenH)}, kPanelColor);
    draw_text_centered(r, 200, "GAME!", kTitleColor);

    const auto& entries = entries_for_state(state_);
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const int y = kMenuTopY + static_cast<int>(i) * kMenuLineDy;
        draw_entry(r, y, entries[i],
                   static_cast<int>(i) == selected_index_,
                   kSettingValX);
    }
}

void Menu::render_settings(Renderer& r) const {
    r.draw_filled_rect({0, 0, static_cast<float>(kScreenW),
                        static_cast<float>(kScreenH)}, kBgColor);
    draw_text_centered(r, 160, "SETTINGS", kTitleColor);

    const auto& entries = settings_entries_;
    constexpr int kVisibleRows = 9;
    constexpr int kSettingsTopY = 225;
    constexpr int kSettingsLineDy = 40;
    const int max_first = std::max(0, static_cast<int>(entries.size()) - kVisibleRows);
    const int first = std::clamp(selected_index_ - kVisibleRows / 2, 0, max_first);
    const int last = std::min(static_cast<int>(entries.size()), first + kVisibleRows);
    for (int i = first; i < last; ++i) {
        const int y = kSettingsTopY + (i - first) * kSettingsLineDy;
        draw_entry(r, y, entries[static_cast<std::size_t>(i)],
                   i == selected_index_, kSettingValX);
    }
    char position[40];
    std::snprintf(position, sizeof(position), "%d / %zu",
                  selected_index_ + 1, entries.size());
    draw_text_centered(r, 190, position, kHintColor);
    draw_text_centered(r, kHintY, "UP/DOWN: SCROLL   LEFT/RIGHT: ADJUST   ENTER: SELECT   ESC: BACK",
                       kHintColor);
}

void Menu::render_controls(Renderer& r) const {
    r.draw_filled_rect({0, 0, static_cast<float>(kScreenW),
                        static_cast<float>(kScreenH)}, kBgColor);
    draw_text_centered(r, 120, "CONTROLS", kTitleColor);

    // Two columns side by side: P1 on the left, P2 on the right.
    // Each column is half-width. We render labels only (values
    // are baked into the label text).
    const auto& entries = controls_entries_;
    int y_p1 = 200;
    int y_p2 = 200;
    constexpr int kColP1X = 200;
    constexpr int kColP2X = 760;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const Entry& e = entries[i];
        const bool is_p1 = (e.label.compare(0, 2, "P1") == 0);
        const bool is_p2 = (e.label.compare(0, 2, "P2") == 0);
        const bool is_meta = !(is_p1 || is_p2);
        int& y = is_p1 ? y_p1 : (is_p2 ? y_p2 : y_p1);
        const int x = is_p1 ? kColP1X : (is_p2 ? kColP2X : kSettingLeftX);
        const config::Color c = (static_cast<int>(i) == selected_index_)
                                ? kFocusColor
                                : config::kHudTextColor;
        ui::draw_text(r, x, y, e.label.c_str(), c);
        if (!e.value.empty()) {
            ui::draw_text(r, x + 220, y, e.value.c_str(), c);
        }
        if (static_cast<int>(i) == selected_index_) {
            r.draw_filled_rect({static_cast<float>(x - 24),
                                static_cast<float>(y + 6), 8.0f, 8.0f},
                               kFocusColor);
        }
        y += 24;
        if (is_meta) {
            y_p1 = y;
            y_p2 = y;
        }
    }
    draw_text_centered(r, kHintY, "ESC / GAMEPAD B: BACK", kHintColor);
}

void Menu::render_remap_capture(Renderer& r) const {
    // Translucent background so the player can still see the
    // settings list behind the prompt (useful for spotting the
    // row they are editing). The "PRESS A KEY" line pulses so
    // the screen feels alive; a static prompt is easy to miss
    // when the user has just pressed confirm.
    r.draw_filled_rect({0, 0, static_cast<float>(kScreenW),
                        static_cast<float>(kScreenH)}, kBgColor);
    draw_text_centered(r, 220, "REBIND", kTitleColor);

    // The slot index identifies which action is being edited.
    // The label is reconstructed here so the UI does not need
    // to keep a separate "currently editing" string.
    const char* action_name = "?";
    switch (remap_slot_) {
        case 0:  action_name = "MOVE LEFT";   break;
        case 1:  action_name = "MOVE RIGHT";  break;
        case 2:  action_name = "JUMP";        break;
        case 3:  action_name = "FAST FALL";   break;
        case 4:  action_name = "JAB";         break;
        case 5:  action_name = "FORWARD TILT";break;
        case 6:  action_name = "NEUTRAL AIR"; break;
        case 7:  action_name = "AIM UP"; break;
        case 8:  action_name = "SPECIAL"; break;
        case 9:  action_name = "SMASH MODIFIER"; break;
        case 10: action_name = "SHIELD"; break;
        case 11: action_name = "DODGE"; break;
        case 12: action_name = "EMBER SURGE"; break;
    }
    const std::string line = std::string(remap_player_ == 0 ? "P1 " : "P2 ") + action_name;
    draw_text_centered(r, 300, line, config::kHudTextColor);

    // Pulse the prompt.
    const int period = 30;
    const int phase  = remap_pulse_ticks_ % period;
    const int bright = (phase < period / 2) ? phase : (period - phase);
    const config::Color prompt {
        static_cast<std::uint8_t>(120 + bright * 2),
        static_cast<std::uint8_t>(120 + bright * 2),
        static_cast<std::uint8_t>(120 + bright * 2),
        255 };
    draw_text_centered(r, 380, "PRESS A KEY", prompt);
    draw_text_centered(r, 420, "ESC TO CANCEL", kHintColor);
}

void Menu::render_quit_confirm(Renderer& r) const {
    r.draw_filled_rect({0, 0, static_cast<float>(kScreenW),
                        static_cast<float>(kScreenH)}, kBgColor);
    draw_text_centered(r, kQuitConfirmY - 80, "QUIT GAME?", kTitleColor);

    const char* yes = "YES";
    const char* no  = "CANCEL";
    const int x_yes = kScreenW / 2 - 80;
    const int x_no  = kScreenW / 2 + 20;

    ui::draw_text(r, x_yes, kQuitConfirmY, yes,
                  selected_index_ == 0 ? kFocusColor : config::kHudTextColor);
    ui::draw_text(r, x_no,  kQuitConfirmY, no,
                  selected_index_ == 1 ? kFocusColor : config::kHudTextColor);
    if (selected_index_ == 0) {
        r.draw_filled_rect({static_cast<float>(x_yes - 24),
                            static_cast<float>(kQuitConfirmY + 6), 8.0f, 8.0f},
                           kFocusColor);
    } else {
        r.draw_filled_rect({static_cast<float>(x_no - 24),
                            static_cast<float>(kQuitConfirmY + 6), 8.0f, 8.0f},
                           kFocusColor);
    }
    draw_text_centered(r, kHintY, "LEFT/RIGHT: CHOOSE   ENTER: CONFIRM", kHintColor);
}

void Menu::render(Renderer& r) const {
    switch (state_) {
        case State::kTitle:        render_title(r);       break;
        case State::kMain:
        case State::kModeSelect:
        case State::kArenaSelect:  render_main(r);        break;
        case State::kPaused:       render_paused(r);      break;
        case State::kRematch:      render_rematch(r);     break;
        case State::kSettings:     render_settings(r);    break;
        case State::kControls:     render_controls(r);    break;
        case State::kQuitConfirm:  render_quit_confirm(r);break;
        case State::kRemapCapture: render_remap_capture(r);break;
        case State::kPlay:         /* nothing; Match renders */ break;
    }
}

} // namespace ember
