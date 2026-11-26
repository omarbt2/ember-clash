#pragma once

#include "Core/Math.h"
#include "Gameplay/AiConfig.h"
#include "Gameplay/Attack.h"
#include "Physics/Motion.h"

#include <cstdint>

namespace ember {

class Arena;
class Fighter;

// -----------------------------------------------------------------------------
// Deterministic finite-state-machine AI driver.
//
// One AiController owns the decision logic for a single fighter. Each
// Match tick the Match calls decide(self, opponent, arena) and uses
// the returned intents in place of InputMap::sample(). The controller
// holds the FSM, the difficulty knobs, and an xorshift32 RNG for the
// "die rolls" the FSM needs (attack commit, double-jump, etc.).
//
// The controller is read-only with respect to the fighters and arena:
// it inspects their public state and produces PlayerIntent + CombatIntent.
// All mutations of self happen via the standard Fighter::update entry
// point, exactly the same way a human-controlled fighter is driven. This
// means AI and human paths are interchangeable and a future "Player 2
// is human" toggle in a rematch menu is a one-liner.
// -----------------------------------------------------------------------------
class AiController {
public:
    // The state names. Exposed for the headless test harness, which counts
    // state transitions to assert the bot actually plays.
    enum class State {
        Idle,       // do nothing; re-evaluate every reaction_delay_ticks
        Approach,   // move toward the opponent
        Attack,     // commit to an attack; wait for swing to end
        Retreat,    // back off (own damage high, or after a trade)
        Recover,    // launched off-stage; jump back to the platform
        Defend,     // wait through opponent's windup
    };

    // Build a controller with the given difficulty. The RNG is seeded from
    // difficulty.seed so two matches with the same seed play identically.
    explicit AiController(ai::Difficulty difficulty = ai::kStandard);

    // One decision. The Match calls this per AI-controlled fighter per tick.
    // The returned intents feed Fighter::update exactly like InputMap's
    // output does for a human. Behaviour windows are integer simulation
    // ticks; no variable-delta time enters the AI state machine.
    void decide(const Fighter& self,
                const Fighter& opponent,
                const Arena& arena,
                int    match_startup_ticks,
                PlayerIntent& out_motion,
                CombatIntent& out_combat);

    // Test / debug accessors. Cheap inline getters so the headless harness
    // can count state visits and verify determinism without poking at
    // private fields.
    State            state()        const { return state_; }
    int              state_ticks()  const { return state_ticks_; }
    std::uint32_t    rng_state()    const { return rng_; }
    int              last_attack_tick() const { return last_attack_tick_; }
    bool             skip_grace_for_test() const { return force_skip_grace_for_test_; }
    const ai::Difficulty& difficulty() const { return difficulty_; }

    // Test-only: reset the controller to its initial state (keeps the
    // difficulty knobs). Used between headless test cases.
    void reset_for_test() { state_ = State::Idle; state_ticks_ = 0; rng_ = difficulty_.seed; last_attack_tick_ = -1; }
    void set_runtime_state_for_test(State state, int ticks, int last_attack_tick,
                                    std::uint32_t rng_state) {
        state_ = state;
        state_ticks_ = ticks;
        last_attack_tick_ = last_attack_tick;
        rng_ = rng_state;
    }

    // P15: test-only bypass. The post-FIGHT startup grace is a UX
    // guarantee for the player; tests that drive the AI in a tight
    // loop (M7-AI Defend tests use 90 frames) want the bot to act
    // immediately so the state machine reaches Defend within the
    // test window. The force flag is a hard opt-in: production code
    // never sets it.
    void set_skip_grace_for_test(bool skip) { force_skip_grace_for_test_ = skip; }

private:
    ai::Difficulty difficulty_;
    State          state_{State::Idle};
    int        state_ticks_{0};
    // P15: global match-time counter at the moment of the last
    // committed attack. The bot refuses to attack again until
    // kAiAttackCooldownTicks have passed (measured against the
    // match_startup_ticks value passed into decide()). -1 means
    // "never attacked" so the very first commit is always
    // allowed.
    int        last_attack_tick_{-1};
    std::uint32_t rng_{0};
    // Test-only: see set_skip_grace_for_test.
    bool force_skip_grace_for_test_{false};

    // xorshift32. Returns 0..2^32-1, then we scale to 0..99 with % 100.
    std::uint32_t next_rand();
    int rand_percent();

    // Distance / situation predicates. All take world-space geometry only.
    static float body_center_distance(const Fighter& a, const Fighter& b);
    static float horizontal_gap(const Fighter& a, const Fighter& b);
    static bool  in_jab_range  (const Fighter& self, const Fighter& opp);
    static bool  in_ftilt_range(const Fighter& self, const Fighter& opp);
    static bool  in_nair_range (const Fighter& self, const Fighter& opp);
    static bool  is_off_stage  (const Fighter& self, const Arena& arena);

    // State-handler methods. Each returns nothing and mutates the two
    // out_intent references; transitions to a new state are signalled by
    // the per-state transition helpers (see cpp). tick_approach takes an
    // extra `match_startup_ticks` so it can record the timestamp of a
    // committed attack for the post-attack cooldown gate.
    void tick_idle    (const Fighter& self, const Fighter& opp, const Arena& arena,
                       PlayerIntent& m, CombatIntent& c);
    void tick_approach(const Fighter& self, const Fighter& opp, const Arena& arena,
                       PlayerIntent& m, CombatIntent& c, int match_startup_ticks);
    void tick_attack  (const Fighter& self, const Fighter& opp, const Arena& arena,
                       PlayerIntent& m, CombatIntent& c);
    void tick_retreat (const Fighter& self, const Fighter& opp, const Arena& arena,
                       PlayerIntent& m, CombatIntent& c);
    void tick_recover (const Fighter& self, const Fighter& opp, const Arena& arena,
                       PlayerIntent& m, CombatIntent& c);
    void tick_defend  (const Fighter& self, const Fighter& opp, const Arena& arena,
                       PlayerIntent& m, CombatIntent& c);

    // Transitions. Setting state_ + state_ticks_ = 0 in one place makes
    // every state change observable to the harness.
    void enter_state(State s) { state_ = s; state_ticks_ = 0; }
};

} // namespace ember
