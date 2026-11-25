# Ember Clash

Ember Clash is an original 2D platform-fighting game built in C++20 with SDL3 and CMake.

The project focuses on responsive movement, readable combat, damage-based knockback, platform recovery, and a deterministic computer-controlled opponent. It is developed without a traditional game engine.

## Technology

- C++20
- SDL3
- CMake
- Ninja or another supported CMake generator

## Requirements

- C++20 compiler (MSVC, Clang, or GCC)
- CMake 3.20 or newer
- Ninja (recommended)
- Windows 10+, Linux, or macOS
- On Windows, the Windows SDK and a supported MSVC toolchain

SDL3 is fetched by CMake during configuration.

## Build on Windows

From an x64 Native Tools Command Prompt for Visual Studio:

```bat
cmake -S . -B build -G Ninja
cmake --build build
```

The executable is produced at `build\bin\EmberClash.exe`.

## Build with Visual Studio

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## Build on Linux or macOS

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/bin/EmberClash
```

## Run

On Windows:

```bat
build\bin\EmberClash.exe
```

The default match is a human player against the computer opponent. Keyboard controls are shown below.

| Action | Player 1 |
|---|---|
| Move | A / D |
| Jump / double jump | W / Space |
| Fast fall | S |
| Jab | J |
| Forward tilt | K |
| Neutral air | L |
| Aim up | I |
| Special | U |
| Smash modifier | Left Shift |
| Shield / parry | O |
| Dodge | P |
| Ember Surge | Y |
| Pause | Esc / Gamepad Start |
| Rematch | R |
| Quit | Esc |

All 13 fighter actions can be rebound per player in Settings. The P2 arrow-key / numpad profile is configurable, but a selectable local two-player mode is not yet implemented; P2 is still the AI opponent in the current release scope.

## Tests

On Windows with the Visual Studio toolchain configured:

```bat
tests\verify_all_desktop_tests.bat
```

Individual regression harnesses are available in `tests/`.

## Project structure

- `src/Core/` — application lifecycle, timing, configuration, and settings
- `src/Gameplay/` — fighters, attacks, AI, match flow, and animation
- `src/Physics/` — movement and physics tuning
- `src/Graphics/` and `src/UI/` — rendering and interface
- `src/Audio/` — audio systems
- `tests/` — headless regression harnesses and Windows test runners
- `web/` — WebAssembly deployment files

## Development

Keep gameplay rules independent from platform-specific input and rendering. Changes to movement, combat, or match flow should include regression coverage. Build output and local diagnostic files should remain out of version control.
