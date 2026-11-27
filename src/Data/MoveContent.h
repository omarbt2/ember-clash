#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <vector>

namespace ember::data {

enum class HitboxShape : unsigned char { Aabb, Circle };

struct HitboxKeyframe {
    int t0 = 0;
    int t1 = 0; // Inclusive tick range.
    HitboxShape shape = HitboxShape::Aabb;
    float offset_x = 0.0f;
    float offset_y = 0.0f;
    float size_x = 0.0f;
    float size_y = 0.0f;
    int damage = 0;
    int angle = 0;
    int base_knockback = 0;
    int knockback_scale = 0;
    int hitstun = 0;
    int group = 0;
    std::string effect;
};

struct CancelWindow {
    std::vector<std::string> targets;
    int from_tick = 0;
    int to_tick = 0;
};

struct MoveDefinition {
    std::string id;
    std::string category;
    int startup = 0;
    int active = 0;
    int recovery = 0;
    std::vector<HitboxKeyframe> hitboxes;
    CancelWindow cancel;
    bool has_cancel = false;
};

struct MoveCatalog {
    int schema = 0;
    std::uint64_t content_hash = 0;
    std::vector<MoveDefinition> moves;
    std::vector<std::string> warnings;
};

struct ContentError {
    std::string source;
    std::string message;
    std::size_t line = 0;
    std::size_t column = 0;
};

// Both operations are transactional: output is unchanged unless every move
// and every cross-reference in the catalog validates successfully.
bool parse_move_catalog(std::string_view text, MoveCatalog& output,
                        ContentError& error,
                        std::string source = "<memory>");
// Finds the catalog in launch working-directory, executable-directory, then
// source-tree order. If no candidate exists, returns the working-directory
// candidate so load_move_catalog reports the expected path in its error.
std::string resolve_move_catalog_path(std::string_view working_directory,
                                      std::string_view executable_directory,
                                      std::string_view source_directory);

bool load_move_catalog(const std::string& path, MoveCatalog& output,
                       ContentError& error);

} // namespace ember::data
