@echo off
setlocal enableextensions

REM ============================================================================
REM M6-B test runner (Windows, MSVC + Ninja, /MDd, loads vcvars64).
REM
REM Builds src/Gameplay/{Animation, Sprite}.cpp + ember_test_m6b.cpp into a
REM console exe at build\ember_test_m6b.exe and runs it. Used by
REM verify_all_desktop_tests.bat as the M6-B regression.
REM
REM This runner is *narrower* than the M4 / M5 runners: it does not link
REM Fighter, Match, Renderer, or the AI controller because the test only
REM exercises Sprite, ArtConfig, and anim::Driver. The drivers are header-
REM only or referenced by name from Sprite's tests; Sprite's pose generators
REM use art::FighterPalette which is in ArtConfig.h.
REM
REM Heads up: this runner hard-codes /MDd (debug C runtime), same as
REM run_ai_test.bat / run_m5_test.bat / run_m6_test.bat. The project must
REM be configured with -DCMAKE_BUILD_TYPE=Debug or /MDd will conflict with
REM the main build's /MD and the test exe will fail to start with
REM LNK2038 RuntimeLibrary mismatch. (Documented in AGENTS.md.)
REM ============================================================================

set "REPO=%~dp0.."
set "BUILD=%REPO%\build"
set "TESTS=%REPO%\tests"

if not exist "%BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Sprite.cpp.obj" (
    echo [M6-B] build tree missing - run tests\verify_desktop_build.bat first.
    exit /b 1
)

set "INCLUDES=/I"%REPO%\src" /I"%BUILD%\_deps\sdl3-src\include" /I"%BUILD%\_deps\sdl3-build\include-revision""

REM The test references Driver::tick, frames_in_pose, ticks_per_frame_for.
REM Those live in Animation.cpp; Sprite::build_animation_strip lives in
REM Sprite.cpp. (ArtConfig is header-only.)
set "OBJS=%BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Animation.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Sprite.cpp.obj"

set "SDL3=%BUILD%\_deps\sdl3-build\SDL3.lib"

if not exist "%BUILD%\obj_test" mkdir "%BUILD%\obj_test"

cl /nologo /EHsc /std:c++latest /W4 /permissive- /utf-8 /Zi /MDd %INCLUDES% /Fo"%BUILD%\obj_test\ember_test_m6b.obj" /Fe"%BUILD%\ember_test_m6b.exe" "%TESTS%\ember_test_m6b.cpp" /link %OBJS% %SDL3%
if errorlevel 1 (
    echo [M6-B] cl failed.
    exit /b 1
)

echo --- M6-B harness running ---
"%BUILD%\ember_test_m6b.exe"
set "RC=%errorlevel%"
echo --- M6-B harness exit %RC% ---
exit /b %RC%
