#pragma once

#include "World/Arena.h"

#include <array>
#include <memory>

namespace ember {

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
    // seconds since the last update.
    void update(const InputManager& input, double dt);

    // Draws the arena + fighters in render-space coordinates.
    void render(Renderer& renderer) const;

    // For tests and the future rematch flow.
    void reset();

    const Arena& arena() const { return arena_; }
    const Fighter& fighter(int index) const;

private:
    Arena arena_;
    std::array<std::unique_ptr<Fighter>, kFighterCount> fighters_;
};

} // namespace ember
