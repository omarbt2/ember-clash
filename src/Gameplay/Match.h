#pragma once

#include "World/Arena.h"

#include <array>
#include <memory>

namespace ember {

class AiController;
class Fighter;
class InputManager;
class Renderer;

// Owns the static arena and the two fighters; per-frame orchestration lives
// here so App can stay a thin driver. Combat, damage, KO counting, and bot
// AI all plug into Match in later milestones.
class Match {
public:
    static constexpr int kFighterCount = 2;

    Match();
    ~Match();

    Match(const Match&)            = delete;
    Match& operator=(const Match&) = delete;

    // One match tick. `input` provides keyboard state; dt is the elapsed
    // seconds since the last update. Fighters with an attached AiController
    // are driven by the AI instead of InputMap.
    void update(const InputManager& input, double dt);

    // Draws the arena + fighters in render-space coordinates.
    void render(Renderer& renderer) const;

    // For tests and the future rematch flow.
    void reset();

    // Per-fighter AI control. Setting a controller for an index makes that
    // fighter AI-driven; the default (no controller) means human-controlled.
    // Used by the headless AI harness and by the future difficulty-select
    // screen.
    void set_ai(int fighter_index, std::unique_ptr<AiController> controller);
    bool is_ai(int fighter_index) const;

    const Arena& arena() const { return arena_; }
    const Fighter& fighter(int index) const;
    // Non-const fighter accessor. Used by the headless test harness to
    // inject damage / position / state without going through combat. Not
    // used in the production update path.
    Fighter&       fighter(int index)       { return *fighters_[index]; }
    const AiController* ai(int fighter_index) const;

private:
    Arena arena_;
    std::array<std::unique_ptr<Fighter>, kFighterCount> fighters_;
    std::array<std::unique_ptr<AiController>, kFighterCount> ai_;
};

} // namespace ember
