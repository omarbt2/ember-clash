#include "Core/Settings.h"

#include "Core/Logger.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

namespace ember {

namespace {

// Trims leading and trailing ASCII whitespace.
std::string trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

// Reads a value of a known type from "key=value" line, returning
// false if the line is malformed.
bool read_int(const std::string& line, const std::string& key, int& out) {
    const std::string prefix = key + "=";
    if (line.rfind(prefix, 0) != 0) return false;
    try {
        out = std::stoi(line.substr(prefix.size()));
        return true;
    } catch (...) {
        return false;
    }
}
bool read_bool(const std::string& line, const std::string& key, bool& out) {
    const std::string prefix = key + "=";
    if (line.rfind(prefix, 0) != 0) return false;
    const std::string v = trim(line.substr(prefix.size()));
    if (v == "1" || v == "true" || v == "yes" || v == "on")  { out = true;  return true; }
    if (v == "0" || v == "false" || v == "no"  || v == "off") { out = false; return true; }
    return false;
}
bool is_supported_keycode(std::int32_t value) {
    if (value == 0) return true; // Zero is retained as an intentionally unbound slot.
    if (value >= 32 && value < 127) return true; // Printable ASCII.
    if (value >= 160 && value < 512) return true; // Printable Latin-1/Unicode range.
    constexpr std::uint32_t kScancodeBit = 1u << 30;
    const std::uint32_t raw = static_cast<std::uint32_t>(value);
    if ((raw & kScancodeBit) == 0) return false;
    const std::uint32_t scancode = raw & ~kScancodeBit;
    return scancode > 0 && scancode < 512;
}

bool read_str(const std::string& line, const std::string& key,
              std::array<char, 16>& out) {
    const std::string prefix = key + "=";
    if (line.rfind(prefix, 0) != 0) return false;
    const std::string v = trim(line.substr(prefix.size()));
    if (v.empty() || v.size() >= out.size()) return false;
    std::memset(out.data(), 0, out.size());
    std::memcpy(out.data(), v.data(), v.size());
    return true;
}

void write_int  (std::ostringstream& o, const std::string& k, int  v) { o << k << "=" << v   << "\n"; }
void write_bool (std::ostringstream& o, const std::string& k, bool v) { o << k << "=" << (v ? "1" : "0") << "\n"; }
void write_str  (std::ostringstream& o, const std::string& k, const std::array<char, 16>& v) {
    o << k << "=";
    o.write(v.data(), strnlen(v.data(), v.size()));
    o << "\n";
}

} // namespace

void Settings::reset_to_defaults() {
    master_volume = 80;
    sfx_volume    = 80;
    music_volume  = 60;
    master_mute   = false;
    sfx_mute      = false;
    music_mute    = false;
    fullscreen    = false;
    render_scale  = 1;
    screen_shake  = true;
    color_blind_palette = false;

    const char* keyboard = "keyboard";
    const char* standard = "standard";
    std::memset(p1_scheme.data(), 0, p1_scheme.size());
    std::memcpy(p1_scheme.data(), keyboard, std::strlen(keyboard));
    std::memset(p2_scheme.data(), 0, p2_scheme.size());
    std::memcpy(p2_scheme.data(), keyboard, std::strlen(keyboard));
    std::memset(ai_difficulty.data(), 0, ai_difficulty.size());
    std::memcpy(ai_difficulty.data(), standard, std::strlen(standard));

    // P1 default bindings. The integers are the SDL_Keycode values
    // for A / D / W / S / J / K / L. We hardcode the literals here
    // (rather than including <SDL3/SDL_keycode.h>) so Settings
    // stays a leaf module with no SDL dependency. The mapping
    // from index to action is:
    //   0 = move_left,  1 = move_right,
    //   2 = jump,       3 = fast_fall,
    //   4 = jab,        5 = ftilt,       6 = nair
    // 7 = up (I), 8 = special (U), 9 = smash (LSHIFT), 10 = shield (O),
    // 11 = dodge (P), 12 = surge (Y).
    p1_bindings = {{97, 100, 119, 115, 106, 107, 108,
                    105, 117, 1073742049, 111, 112, 121}};

    // P2 defaults: arrow-keys + numpad (distinct from P1's
    // WASD+JKL). The literals are SDL_Keycode values:
    //   LEFT=1073741904, RIGHT=1073741903, UP=1073741906,
    //   DOWN=1073741905, KP_1=1073741913, KP_2=1073741914,
    //   KP_3=1073741915. Hardcoded for the same reason as P1.
    p2_bindings = {{1073741904, 1073741903, 1073741906, 1073741905,
                    1073741913, 1073741914, 1073741915,
                    1073741920, 1073741916, 1073741922, 1073741917,
                    1073741918, 1073741919}};
}

bool Settings::operator==(const Settings& other) const {
    return master_volume  == other.master_volume
        && sfx_volume     == other.sfx_volume
        && music_volume   == other.music_volume
        && master_mute    == other.master_mute
        && sfx_mute       == other.sfx_mute
        && music_mute     == other.music_mute
        && fullscreen     == other.fullscreen
        && render_scale   == other.render_scale
        && screen_shake   == other.screen_shake
        && color_blind_palette == other.color_blind_palette
        && p1_scheme      == other.p1_scheme
        && p2_scheme      == other.p2_scheme
        && ai_difficulty  == other.ai_difficulty
        && p1_bindings    == other.p1_bindings
        && p2_bindings    == other.p2_bindings;
}

Settings load_settings_from_file(const std::string& path) {
    Settings s;
    s.reset_to_defaults();

    std::ifstream in(path);
    if (!in.is_open()) {
        // First launch or unreadable file. Defaults are fine.
        return s;
    }

    std::string line;
    while (std::getline(in, line)) {
        const std::string t = trim(line);
        if (t.empty() || t[0] == '#') continue;

        // Each parse attempt is best-effort; an unrecognised key is
        // simply ignored so a future build that adds new fields can
        // still read the older file format.
        if      (read_int (t, "master_volume",  s.master_volume))  {}
        else if (read_int (t, "sfx_volume",     s.sfx_volume))     {}
        else if (read_int (t, "music_volume",   s.music_volume))   {}
        else if (read_bool(t, "master_mute",    s.master_mute))    {}
        else if (read_bool(t, "sfx_mute",       s.sfx_mute))       {}
        else if (read_bool(t, "music_mute",     s.music_mute))     {}
        else if (read_bool(t, "fullscreen",     s.fullscreen))     {}
        else if (read_int (t, "render_scale",   s.render_scale))   {}
        else if (read_bool(t, "screen_shake",   s.screen_shake))   {}
        else if (read_bool(t, "color_blind",    s.color_blind_palette)) {}
        else if (read_str (t, "p1_scheme",      s.p1_scheme))      {}
        else if (read_str (t, "p2_scheme",      s.p2_scheme))      {}
        else if (read_str (t, "ai_difficulty",  s.ai_difficulty))  {}
        // P1 key bindings. Each is a raw 32-bit SDL_Keycode value.
        // Values are validated after parsing; malformed keycodes
        // fall back to the corresponding default action binding.
        else if (read_int (t, "p1_move_left",   s.p1_bindings[0])) {}
        else if (read_int (t, "p1_move_right",  s.p1_bindings[1])) {}
        else if (read_int (t, "p1_jump",        s.p1_bindings[2])) {}
        else if (read_int (t, "p1_fast_fall",   s.p1_bindings[3])) {}
        else if (read_int (t, "p1_jab",         s.p1_bindings[4])) {}
        else if (read_int (t, "p1_ftilt",       s.p1_bindings[5])) {}
        else if (read_int (t, "p1_nair",        s.p1_bindings[6])) {}
        else if (read_int (t, "p1_up",           s.p1_bindings[7])) {}
        else if (read_int (t, "p1_special",      s.p1_bindings[8])) {}
        else if (read_int (t, "p1_smash",        s.p1_bindings[9])) {}
        else if (read_int (t, "p1_shield",       s.p1_bindings[10])) {}
        else if (read_int (t, "p1_dodge",        s.p1_bindings[11])) {}
        else if (read_int (t, "p1_surge",        s.p1_bindings[12])) {}
        // P2 key bindings. M13.1: same shape as P1. Older
        // settings files (M13 and earlier) have no P2 keys; the
        // p2_bindings array is left at its reset_to_defaults
        // values in that case.
        else if (read_int (t, "p2_move_left",   s.p2_bindings[0])) {}
        else if (read_int (t, "p2_move_right",  s.p2_bindings[1])) {}
        else if (read_int (t, "p2_jump",        s.p2_bindings[2])) {}
        else if (read_int (t, "p2_fast_fall",   s.p2_bindings[3])) {}
        else if (read_int (t, "p2_jab",         s.p2_bindings[4])) {}
        else if (read_int (t, "p2_ftilt",       s.p2_bindings[5])) {}
        else if (read_int (t, "p2_nair",        s.p2_bindings[6])) {}
        else if (read_int (t, "p2_up",           s.p2_bindings[7])) {}
        else if (read_int (t, "p2_special",      s.p2_bindings[8])) {}
        else if (read_int (t, "p2_smash",        s.p2_bindings[9])) {}
        else if (read_int (t, "p2_shield",       s.p2_bindings[10])) {}
        else if (read_int (t, "p2_dodge",        s.p2_bindings[11])) {}
        else if (read_int (t, "p2_surge",        s.p2_bindings[12])) {}
        // Unknown key: silently ignored.
    }

    // Clamp to legal ranges. A corrupted file with garbage values
    // must not let the game boot with a phantom volume of 9999.
    if (s.master_volume < 0)   s.master_volume = 0;
    if (s.master_volume > 100) s.master_volume = 100;
    if (s.sfx_volume < 0)      s.sfx_volume = 0;
    if (s.sfx_volume > 100)    s.sfx_volume = 100;
    if (s.music_volume < 0)    s.music_volume = 0;
    if (s.music_volume > 100)  s.music_volume = 100;
    if (s.render_scale < 1)    s.render_scale = 1;
    if (s.render_scale > 4)    s.render_scale = 4;
    // AI difficulty is a stable preset identifier. A hand-edited or
    // partially corrupted settings file must not leave the menu displaying
    // an unsupported tier while runtime silently falls back to Standard.
    const std::string difficulty(s.ai_difficulty.data());
    if (difficulty != "easy" && difficulty != "standard" &&
        difficulty != "hard" && difficulty != "expert") {
        s.ai_difficulty.fill(0);
        std::memcpy(s.ai_difficulty.data(), "standard", 8);
    }

    // Keep persisted values inside the same representable keycode space as
    // InputManager: plain ASCII and SDL's scancode-masked keycodes. Zero is
    // retained as an intentionally unbound slot; malformed values fall back
    // to that action's default instead of aliasing the invalid-key sentinel.
    Settings defaults;
    defaults.reset_to_defaults();
    for (std::size_t i = 0; i < s.p1_bindings.size(); ++i) {
        if (!is_supported_keycode(s.p1_bindings[i])) {
            s.p1_bindings[i] = defaults.p1_bindings[i];
        }
        if (!is_supported_keycode(s.p2_bindings[i])) {
            s.p2_bindings[i] = defaults.p2_bindings[i];
        }
    }

    return s;
}

bool save_settings_to_file(const std::string& path, const Settings& s) {
    // Best-effort atomic write: write to "<path>.tmp" first, then
    // rename over the real file. A crash mid-write leaves the old
    // file intact (or no file) instead of a half-written one that
    // would fail to parse.
    std::ostringstream o;
    o << "# Ember Clash settings - auto-generated, do not edit by hand\n";
    write_int (o, "master_volume",  s.master_volume);
    write_int (o, "sfx_volume",     s.sfx_volume);
    write_int (o, "music_volume",   s.music_volume);
    write_bool(o, "master_mute",    s.master_mute);
    write_bool(o, "sfx_mute",       s.sfx_mute);
    write_bool(o, "music_mute",     s.music_mute);
    write_bool(o, "fullscreen",     s.fullscreen);
    write_int (o, "render_scale",   s.render_scale);
    write_bool(o, "screen_shake",   s.screen_shake);
    write_bool(o, "color_blind",    s.color_blind_palette);
    write_str (o, "p1_scheme",      s.p1_scheme);
    write_str (o, "p2_scheme",      s.p2_scheme);
    write_str (o, "ai_difficulty",  s.ai_difficulty);
    write_int (o, "p1_move_left",   s.p1_bindings[0]);
    write_int (o, "p1_move_right",  s.p1_bindings[1]);
    write_int (o, "p1_jump",        s.p1_bindings[2]);
    write_int (o, "p1_fast_fall",   s.p1_bindings[3]);
    write_int (o, "p1_jab",         s.p1_bindings[4]);
    write_int (o, "p1_ftilt",       s.p1_bindings[5]);
    write_int (o, "p1_nair",        s.p1_bindings[6]);
    write_int (o, "p1_up",           s.p1_bindings[7]);
    write_int (o, "p1_special",      s.p1_bindings[8]);
    write_int (o, "p1_smash",        s.p1_bindings[9]);
    write_int (o, "p1_shield",       s.p1_bindings[10]);
    write_int (o, "p1_dodge",        s.p1_bindings[11]);
    write_int (o, "p1_surge",        s.p1_bindings[12]);
    write_int (o, "p2_move_left",   s.p2_bindings[0]);
    write_int (o, "p2_move_right",  s.p2_bindings[1]);
    write_int (o, "p2_jump",        s.p2_bindings[2]);
    write_int (o, "p2_fast_fall",   s.p2_bindings[3]);
    write_int (o, "p2_jab",         s.p2_bindings[4]);
    write_int (o, "p2_ftilt",       s.p2_bindings[5]);
    write_int (o, "p2_nair",        s.p2_bindings[6]);
    write_int (o, "p2_up",           s.p2_bindings[7]);
    write_int (o, "p2_special",      s.p2_bindings[8]);
    write_int (o, "p2_smash",        s.p2_bindings[9]);
    write_int (o, "p2_shield",       s.p2_bindings[10]);
    write_int (o, "p2_dodge",        s.p2_bindings[11]);
    write_int (o, "p2_surge",        s.p2_bindings[12]);

    const std::string tmp = path + ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            Logger::warn("Settings: failed to open tmp file for write");
            return false;
        }
        out << o.str();
        if (!out.good()) {
            Logger::warn("Settings: write failed");
            return false;
        }
    }
    // Rename. On Windows std::rename will fail if the destination
    // exists, so remove first. The destination may be missing
    // (first launch) which is fine.
    std::remove(path.c_str());
    if (std::rename(tmp.c_str(), path.c_str()) != 0) {
        Logger::warn("Settings: rename failed");
        return false;
    }
    return true;
}

std::string default_settings_path() {
    // For V1.0 we keep the file next to the executable. The
    // Settings dialog reads this and the menu's "save" action
    // writes back to the same path. A future build will switch
    // to %APPDATA%/EmberClash/settings.ini on Windows and
    // ~/.config/ember-clash/settings.ini on Linux; for now the
    // portable-path approach is the easiest to test and to ship.
    return "settings.ini";
}

} // namespace ember
