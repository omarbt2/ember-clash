#include "Data/Json.h"
#include "Data/HitboxEngine.h"
#include "Data/MoveContent.h"
#include "Gameplay/Attack.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>

using namespace ember;

namespace {
constexpr const char* kValid = R"json({
  "schema": 1,
  "moves": [
    {"id":"jab","category":"ground","startup":3,"active":2,"recovery":6,
     "hitboxes":[{"t0":3,"t1":4,"shape":"aabb","off":[12,24],"size":[28,16],
       "damage":4,"angle":18,"base_kb":4,"kb_scale":0,"hitstun":12,"group":0,"fx":"spark"}],
     "cancel":{"from":7,"to_tick":8,"to":["jab2"]},"future_field":{"ignored":true}},
    {"id":"jab2","category":"ground","startup":2,"active":1,"recovery":5,
     "hitboxes":[{"t0":2,"t1":2,"shape":"circle","off":[0,0],"size":[20,20],
       "damage":2,"angle":361,"base_kb":1,"kb_scale":2,"hitstun":4,"group":1}]}
  ]
})json";

bool parse_fails(const std::string& text) {
    data::MoveCatalog catalog;
    data::ContentError error;
    return !data::parse_move_catalog(text, catalog, error, "test.json") && !error.message.empty();
}
}

int main() {
    data::MoveCatalog catalog;
    data::ContentError error;
    assert(data::parse_move_catalog(kValid, catalog, error));
    assert(error.message.empty());
    assert(catalog.schema == 1);
    assert(catalog.moves.size() == 2U);
    assert(catalog.moves[0].id == "jab");
    assert(catalog.moves[0].has_cancel);
    assert(catalog.moves[0].cancel.targets.size() == 1U);
    assert(!catalog.warnings.empty());
    assert(catalog.warnings[0].find("future_field") != std::string::npos);
    assert(catalog.moves[0].hitboxes[0].t0 == 3);
    assert(catalog.moves[1].hitboxes[0].shape == data::HitboxShape::Circle);
    assert(catalog.moves[1].hitboxes[0].angle == 361);

    const Aabb fighter_body{100.0f, 100.0f, 48.0f, 64.0f};
    assert(data::hitboxes_at_tick(catalog.moves[0], 2, fighter_body, 1).empty());
    const auto jab_right = data::hitboxes_at_tick(catalog.moves[0], 3, fighter_body, 1);
    const auto jab_left = data::hitboxes_at_tick(catalog.moves[0], 3, fighter_body, -1);
    assert(jab_right.size() == 1U && jab_left.size() == 1U);
    assert(jab_right[0].bounds.x == 112.0f);
    assert(jab_left[0].bounds.x == 108.0f);
    assert(data::hitboxes_at_tick(catalog.moves[0], 4, fighter_body, 1).size() == 1U);
    assert(data::hitboxes_at_tick(catalog.moves[0], 5, fighter_body, 1).empty());
    assert(data::hitboxes_at_tick(catalog.moves[0], 3, fighter_body, 0).empty());
    assert(data::hitbox_overlaps(jab_right[0], Aabb{139.0f, 128.0f, 3.0f, 4.0f}));
    assert(!data::hitbox_overlaps(jab_right[0], Aabb{141.0f, 128.0f, 3.0f, 4.0f}));

    const auto jab2_circle = data::hitboxes_at_tick(catalog.moves[1], 2, fighter_body, 1);
    assert(jab2_circle.size() == 1U);
    assert(data::hitbox_overlaps(jab2_circle[0], Aabb{119.0f, 105.0f, 4.0f, 4.0f}));
    assert(!data::hitbox_overlaps(jab2_circle[0], Aabb{120.0f, 120.0f, 2.0f, 2.0f}));

    // JSON parser: escaped Unicode (including a surrogate pair) becomes UTF-8.
    data::json::Value unicode;
    data::json::ParseError json_error;
    assert(data::json::parse(R"({"label":"Kaelen \ud83d\udd25"})", unicode, json_error));
    const data::json::Value* label = unicode.find("label");
    assert(label != nullptr && label->as_string() != nullptr);
    assert(label->as_string()->find("Kaelen ") == 0U);

    // Runtime path resolution prefers the launch working directory, then the
    // executable directory (packaged desktop/web assets), then the developer
    // source tree. Each branch is exercised against real temporary files.
    const std::filesystem::path path_root =
        std::filesystem::temp_directory_path() / "ember_move_catalog_path_test";
    std::filesystem::remove_all(path_root);
    const auto working = path_root / "working";
    const auto executable = path_root / "executable";
    const auto source = path_root / "source";
    const auto write_catalog_marker = [](const std::filesystem::path& root) {
        const auto file = root / "assets" / "data" / "moves.json";
        std::filesystem::create_directories(file.parent_path());
        std::ofstream output(file);
        output << "{}";
        assert(output.good());
    };
    write_catalog_marker(executable);
    write_catalog_marker(source);
    assert(data::resolve_move_catalog_path(working.string(), executable.string(),
                                          source.string()) ==
           (executable / "assets" / "data" / "moves.json").string());
    write_catalog_marker(working);
    assert(data::resolve_move_catalog_path(working.string(), executable.string(),
                                          source.string()) ==
           (working / "assets" / "data" / "moves.json").string());
    std::filesystem::remove(working / "assets" / "data" / "moves.json");
    std::filesystem::remove(executable / "assets" / "data" / "moves.json");
    assert(data::resolve_move_catalog_path(working.string(), executable.string(),
                                          source.string()) ==
           (source / "assets" / "data" / "moves.json").string());
    std::filesystem::remove(source / "assets" / "data" / "moves.json");
    assert(data::resolve_move_catalog_path(working.string(), executable.string(),
                                          source.string()) ==
           (working / "assets" / "data" / "moves.json").string());
    std::filesystem::remove_all(path_root);

    // The real starter catalog is validated against the exact current code roster.
    const std::string source_path = std::string(EMBER_SOURCE_DIR) + "/assets/data/moves.json";
    data::MoveCatalog starter;
    assert(data::load_move_catalog(source_path, starter, error));
    assert(starter.content_hash != 0);
    assert(starter.moves.size() == static_cast<std::size_t>(AttackId::Count) - 1U);
    assert(starter.moves.front().id == "jab");
    assert(starter.moves.front().hitboxes.front().knockback_scale == 4);
    assert(starter.moves.back().id == "special_d");

    // A failed operation never partially overwrites an existing catalog.
    const std::string preserved_id = starter.moves.front().id;
    assert(!data::parse_move_catalog("{", starter, error, "broken.json"));
    assert(starter.content_hash != 0);
    assert(starter.moves.size() == static_cast<std::size_t>(AttackId::Count) - 1U);
    assert(starter.moves.front().id == preserved_id);

    assert(parse_fails(R"({"schema":2,"moves":[]})"));                    // unsupported schema
    assert(parse_fails(R"({"schema":1,"moves":[]})"));                    // empty catalog
    assert(parse_fails(R"({"schema":1,"moves":[{"id":"A"}]})"));     // invalid id / missing fields
    assert(parse_fails(R"({"schema":1,"moves":[
      {"id":"jab","category":"ground","startup":3,"active":2,"recovery":6,
       "hitboxes":[{"t0":3,"t1":4,"shape":"aabb","off":[0,0],"size":[1,1],"damage":4,"angle":0,"base_kb":1,"kb_scale":0,"hitstun":4,"group":0}]},
      {"id":"jab","category":"ground","startup":3,"active":2,"recovery":6,
       "hitboxes":[{"t0":3,"t1":4,"shape":"aabb","off":[0,0],"size":[1,1],"damage":4,"angle":0,"base_kb":1,"kb_scale":0,"hitstun":4,"group":0}]}
    ]})"));                                                                   // duplicate move id
    assert(parse_fails(R"({"schema":1,"moves":[
      {"id":"jab","category":"ground","startup":3,"active":2,"recovery":6,
       "hitboxes":[{"t0":3,"t1":4,"shape":"aabb","off":[0,0],"size":[1,1],"damage":4,"angle":0,"base_kb":1,"kb_scale":0,"hitstun":4,"group":0}],
       "cancel":{"from":0,"to_tick":1,"to":["missing"]}}
    ]})"));                                                                   // unresolved cross-reference
    assert(parse_fails(R"({"schema":1,"moves":[
      {"id":"jab","category":"ground","startup":3,"active":2,"recovery":6,
       "hitboxes":[{"t0":4,"t1":5,"shape":"aabb","off":[0,0],"size":[1,1],"damage":4,"angle":0,"base_kb":1,"kb_scale":0,"hitstun":4,"group":0}]}
    ]})"));                                                                   // hitbox outside active window
    assert(parse_fails(R"({"schema":1,"moves":[
      {"id":"jab","category":"ground","startup":3,"active":2,"recovery":6,
       "hitboxes":[{"t0":3,"t1":3,"shape":"circle","off":[0,0],"size":[8,10],"damage":4,"angle":0,"base_kb":1,"kb_scale":0,"hitstun":4,"group":0}]}
    ]})"));                                                                   // circles must be circular

    // Strict JSON grammar: duplicate keys, trailing comma, and incomplete exponent fail.
    data::json::Value ignored;
    assert(!data::json::parse(R"({"schema":1,"schema":1})", ignored, json_error));
    assert(!data::json::parse(R"([1,])", ignored, json_error));
    assert(!data::json::parse(R"(1e+)", ignored, json_error));
    const std::string invalid_utf8 = std::string("\xC0\xAF", 2);
    assert(!data::json::parse(invalid_utf8, ignored, json_error));

    // Exercise the file loader's explicit failure path.
    assert(!data::load_move_catalog(source_path + ".missing", starter, error));
    assert(!error.message.empty());
    return 0;
}
