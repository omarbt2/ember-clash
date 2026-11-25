#pragma once

// -----------------------------------------------------------------------------
// Physics tuning constants.
//
// These are starting-point values from the design brief. They live in one
// place so the simulation layer (milestone 2) can pull them directly without
// hunting through gameplay code. Units are "pixels per simulation tick".
// The simulation runs at config::kTargetFps ticks per second.
// -----------------------------------------------------------------------------

namespace ember::physics {

// Downward acceleration applied each tick while airborne.
inline constexpr double kGravity = 0.8;

// Maximum horizontal walk speed in px/tick.
inline constexpr double kWalkSpeed = 6.0;

// Rebirth movement windows are measured in fixed simulation ticks (60 Hz).
inline constexpr int kJumpBufferTicks = 6;
inline constexpr int kCoyoteTicks = 6;
inline constexpr double kRunSpeedMultiplier = 1.45;

// Vertical velocity applied on the first jump (negative = upward).
inline constexpr double kFirstJumpVelocity = -14.0;

// Vertical velocity applied on the double jump (less powerful).
inline constexpr double kDoubleJumpVelocity = -12.0;

// Air-control coefficient (0..1). Player can change horizontal velocity by
// at most kAirControl * kWalkSpeed each tick while airborne.
inline constexpr double kAirControl = 0.70;

// Terminal downward velocity (fast fall cap).
inline constexpr double kTerminalVelocity = 16.0;

// Multiplier applied to gravity when fast-falling.
inline constexpr double kFastFallMultiplier = 1.5;

} // namespace ember::physics