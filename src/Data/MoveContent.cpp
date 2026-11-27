#include "Data/MoveContent.h"

#include "Data/Json.h"

#include <cmath>
#include <filesystem>
#include <array>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <utility>

namespace ember::data {
namespace {

using json::Value;

void collect_unknown_fields(const Value& object,
                            const std::set<std::string_view>& allowed,
                            const std::string& path,
                            std::vector<std::string>& warnings) {
    const auto* fields = object.as_object();
    if (fields == nullptr) return;
    for (const auto& [name, unused] : *fields) {
        (void)unused;
        if (allowed.find(name) == allowed.end())
            warnings.push_back(path + ": unknown field '" + name + "' (ignored)");
    }
}

bool fail(ContentError& error, const std::string& source, std::string message) {
    error.source = source;
    error.message = std::move(message);
    error.line = 0;
    error.column = 0;
    return false;
}

bool get_required(const Value& object, std::string_view key,
                  const Value*& value, ContentError& error,
                  const std::string& source, const std::string& path) {
    value = object.find(key);
    if (value == nullptr) return fail(error, source, path + ": missing required field '" + std::string(key) + "'");
    return true;
}

bool read_string(const Value& object, std::string_view key, std::string& out,
                 ContentError& error, const std::string& source,
                 const std::string& path) {
    const Value* value = nullptr;
    if (!get_required(object, key, value, error, source, path)) return false;
    const auto* string = value->as_string();
    if (string == nullptr) return fail(error, source, path + "." + std::string(key) + ": expected string");
    out = *string;
    return true;
}

bool read_integer(const Value& object, std::string_view key, int minimum,
                  int maximum, int& out, ContentError& error,
                  const std::string& source, const std::string& path) {
    const Value* value = nullptr;
    if (!get_required(object, key, value, error, source, path)) return false;
    const double* number = value->as_number();
    if (number == nullptr || std::floor(*number) != *number ||
        *number < static_cast<double>(minimum) || *number > static_cast<double>(maximum)) {
        return fail(error, source, path + "." + std::string(key) + ": expected integer in range [" +
                    std::to_string(minimum) + ", " + std::to_string(maximum) + "]");
    }
    out = static_cast<int>(*number);
    return true;
}

bool read_number_pair(const Value& object, std::string_view key,
                      float& x, float& y, ContentError& error,
                      const std::string& source, const std::string& path,
                      float minimum, float maximum, bool positive) {
    const Value* value = nullptr;
    if (!get_required(object, key, value, error, source, path)) return false;
    const auto* array = value->as_array();
    if (array == nullptr || array->size() != 2U)
        return fail(error, source, path + "." + std::string(key) + ": expected a 2-number array");
    const double* first = (*array)[0].as_number();
    const double* second = (*array)[1].as_number();
    const auto valid = [minimum, maximum, positive](const double* number) {
        return number != nullptr && std::isfinite(*number) && *number >= minimum &&
               *number <= maximum && (!positive || *number > 0.0);
    };
    if (!valid(first) || !valid(second))
        return fail(error, source, path + "." + std::string(key) + ": coordinates are outside the supported range");
    x = static_cast<float>(*first);
    y = static_cast<float>(*second);
    return true;
}

bool valid_id(std::string_view id) {
    if (id.empty() || id.size() > 64U) return false;
    for (const char c : id) {
        const bool valid = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
        if (!valid) return false;
    }
    return true;
}

bool valid_category(std::string_view category) {
    return category == "ground" || category == "air" || category == "special" ||
           category == "grab" || category == "throw";
}

bool parse_hitbox(const Value& value, int startup, int active,
                  HitboxKeyframe& out, ContentError& error,
                  const std::string& source, const std::string& path,
                  std::vector<std::string>& warnings) {
    if (value.as_object() == nullptr) return fail(error, source, path + ": expected object");
    collect_unknown_fields(value, {"t0", "t1", "shape", "off", "size", "damage",
                                   "angle", "base_kb", "kb_scale", "hitstun", "group", "fx"},
                           path, warnings);
    if (!read_integer(value, "t0", startup, startup + active - 1, out.t0, error, source, path) ||
        !read_integer(value, "t1", startup, startup + active - 1, out.t1, error, source, path)) return false;
    if (out.t1 < out.t0) return fail(error, source, path + ": t1 must be >= t0");

    std::string shape;
    if (!read_string(value, "shape", shape, error, source, path)) return false;
    if (shape == "aabb") out.shape = HitboxShape::Aabb;
    else if (shape == "circle") out.shape = HitboxShape::Circle;
    else return fail(error, source, path + ".shape: expected 'aabb' or 'circle'");

    if (!read_number_pair(value, "off", out.offset_x, out.offset_y, error, source, path,
                          -1024.0F, 1024.0F, false) ||
        !read_number_pair(value, "size", out.size_x, out.size_y, error, source, path,
                          0.0F, 1024.0F, true)) return false;
    if (out.shape == HitboxShape::Circle && std::fabs(out.size_x - out.size_y) > 0.001F)
        return fail(error, source, path + ".size: circle hitboxes require equal width and height");
    if (!read_integer(value, "damage", 0, 60, out.damage, error, source, path) ||
        !read_integer(value, "angle", 0, 361, out.angle, error, source, path) ||
        !read_integer(value, "base_kb", 0, 300, out.base_knockback, error, source, path) ||
        !read_integer(value, "kb_scale", 0, 100, out.knockback_scale, error, source, path) ||
        !read_integer(value, "hitstun", 0, 180, out.hitstun, error, source, path) ||
        !read_integer(value, "group", 0, 255, out.group, error, source, path)) return false;

    if (const Value* effect = value.find("fx")) {
        const auto* text = effect->as_string();
        if (text == nullptr || text->size() > 64U)
            return fail(error, source, path + ".fx: expected string of at most 64 bytes");
        out.effect = *text;
    }
    return true;
}

bool parse_move(const Value& value, MoveDefinition& out,
                ContentError& error, const std::string& source,
                const std::string& path, std::vector<std::string>& warnings) {
    if (value.as_object() == nullptr) return fail(error, source, path + ": expected object");
    collect_unknown_fields(value, {"id", "category", "startup", "active", "recovery", "hitboxes", "cancel"},
                           path, warnings);
    if (!read_string(value, "id", out.id, error, source, path)) return false;
    if (!valid_id(out.id)) return fail(error, source, path + ".id: expected lowercase [a-z0-9_] id, 1..64 chars");
    if (!read_string(value, "category", out.category, error, source, path)) return false;
    if (!valid_category(out.category))
        return fail(error, source, path + ".category: unsupported category");
    if (!read_integer(value, "startup", 1, 60, out.startup, error, source, path) ||
        !read_integer(value, "active", 1, 60, out.active, error, source, path) ||
        !read_integer(value, "recovery", 0, 120, out.recovery, error, source, path)) return false;
    if (out.startup + out.active + out.recovery > 240)
        return fail(error, source, path + ": total move duration exceeds 240 ticks");

    const Value* hitboxes_value = nullptr;
    if (!get_required(value, "hitboxes", hitboxes_value, error, source, path)) return false;
    const auto* hitboxes = hitboxes_value->as_array();
    if (hitboxes == nullptr || hitboxes->empty() || hitboxes->size() > 64U)
        return fail(error, source, path + ".hitboxes: expected 1..64 hitbox keyframes");
    out.hitboxes.reserve(hitboxes->size());
    for (std::size_t i = 0; i < hitboxes->size(); ++i) {
        HitboxKeyframe keyframe;
        if (!parse_hitbox((*hitboxes)[i], out.startup, out.active, keyframe,
                          error, source, path + ".hitboxes[" + std::to_string(i) + "]", warnings)) return false;
        out.hitboxes.push_back(std::move(keyframe));
    }

    if (const Value* cancel = value.find("cancel")) {
        if (cancel->as_object() == nullptr) return fail(error, source, path + ".cancel: expected object");
        collect_unknown_fields(*cancel, {"from", "to_tick", "to"}, path + ".cancel", warnings);
        out.has_cancel = true;
        if (!read_integer(*cancel, "from", 0, 239, out.cancel.from_tick, error, source, path + ".cancel") ||
            !read_integer(*cancel, "to_tick", 0, 239, out.cancel.to_tick, error, source, path + ".cancel")) return false;
        if (out.cancel.from_tick > out.cancel.to_tick ||
            out.cancel.to_tick >= out.startup + out.active + out.recovery)
            return fail(error, source, path + ".cancel: cancel window must be ordered and inside move duration");
        const Value* targets_value = nullptr;
        if (!get_required(*cancel, "to", targets_value, error, source, path + ".cancel")) return false;
        const auto* targets = targets_value->as_array();
        if (targets == nullptr || targets->empty() || targets->size() > 32U)
            return fail(error, source, path + ".cancel.to: expected 1..32 target ids");
        std::set<std::string> unique_targets;
        for (const Value& target_value : *targets) {
            const auto* target = target_value.as_string();
            if (target == nullptr || !valid_id(*target))
                return fail(error, source, path + ".cancel.to: each target must be a valid move id");
            if (!unique_targets.insert(*target).second)
                return fail(error, source, path + ".cancel.to: duplicate target id");
            out.cancel.targets.push_back(*target);
        }
    }
    return true;
}

} // namespace

bool parse_move_catalog(std::string_view text, MoveCatalog& output,
                        ContentError& error, std::string source) {
    json::Value root;
    json::ParseError parse_error;
    if (!json::parse(text, root, parse_error)) {
        error.source = source;
        error.message = parse_error.message;
        error.line = parse_error.line;
        error.column = parse_error.column;
        return false;
    }
    if (root.as_object() == nullptr) return fail(error, source, "$: root must be an object");

    MoveCatalog candidate;
    // Fingerprint exact source bytes so replay/snapshot diagnostics can detect
    // content revisions even when the schema number remains unchanged.
    candidate.content_hash = 14695981039346656037ULL;
    for (const unsigned char byte : text) {
        candidate.content_hash ^= static_cast<std::uint64_t>(byte);
        candidate.content_hash *= 1099511628211ULL;
    }
    collect_unknown_fields(root, {"schema", "moves"}, "$", candidate.warnings);
    if (!read_integer(root, "schema", 1, 1, candidate.schema, error, source, "$")) return false;
    const Value* moves_value = nullptr;
    if (!get_required(root, "moves", moves_value, error, source, "$")) return false;
    const auto* moves = moves_value->as_array();
    if (moves == nullptr || moves->empty() || moves->size() > 128U)
        return fail(error, source, "$.moves: expected 1..128 move definitions");

    std::set<std::string> ids;
    candidate.moves.reserve(moves->size());
    for (std::size_t i = 0; i < moves->size(); ++i) {
        MoveDefinition move;
        const std::string path = "$.moves[" + std::to_string(i) + "]";
        if (!parse_move((*moves)[i], move, error, source, path, candidate.warnings)) return false;
        if (!ids.insert(move.id).second)
            return fail(error, source, path + ".id: duplicate move id '" + move.id + "'");
        candidate.moves.push_back(std::move(move));
    }

    for (const MoveDefinition& move : candidate.moves) {
        if (!move.has_cancel) continue;
        for (const std::string& target : move.cancel.targets) {
            if (ids.find(target) == ids.end())
                return fail(error, source, "move '" + move.id + "' references missing cancel target '" + target + "'");
        }
    }

    output = std::move(candidate);
    error = {};
    return true;
}

std::string resolve_move_catalog_path(std::string_view working_directory,
                                      std::string_view executable_directory,
                                      std::string_view source_directory) {
    namespace fs = std::filesystem;
    fs::path working_root;
    if (working_directory.empty()) {
        std::error_code current_path_error;
        working_root = fs::current_path(current_path_error);
        if (current_path_error) working_root = fs::path{};
    } else {
        working_root = fs::path(working_directory);
    }

    const fs::path relative_catalog = fs::path("assets") / "data" / "moves.json";
    const fs::path working_candidate = working_root / relative_catalog;
    const fs::path executable_candidate = fs::path(executable_directory) / relative_catalog;
    const fs::path source_candidate = fs::path(source_directory) / relative_catalog;
    const auto is_file = [](const fs::path& path) {
        std::error_code error;
        return fs::is_regular_file(path, error) && !error;
    };

    if (is_file(working_candidate)) return working_candidate.string();
    if (!executable_directory.empty() && is_file(executable_candidate))
        return executable_candidate.string();
    if (!source_directory.empty() && is_file(source_candidate))
        return source_candidate.string();
    return working_candidate.string();
}

bool load_move_catalog(const std::string& path, MoveCatalog& output,
                       ContentError& error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return fail(error, path, "unable to open move catalog");
    std::string text;
    std::array<char, 8192> chunk{};
    while (input) {
        input.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
        const std::streamsize count = input.gcount();
        if (count > 0) {
            const auto bytes = static_cast<std::size_t>(count);
            if (text.size() > json::kMaxInputBytes - bytes)
                return fail(error, path, "JSON input exceeds 4 MiB limit");
            text.append(chunk.data(), bytes);
        }
    }
    if (!input.eof()) return fail(error, path, "failed while reading move catalog");
    return parse_move_catalog(text, output, error, path);
}

} // namespace ember::data
