@echo off
REM ==========================================================================
REM Ember Clash P17 headless test runner (Windows).
REM
REM Companion to run_m8_test.bat. P17 ("Human Playability Recovery +
REM Final System Completion") fixed the main-loop dt-scaling issue by
REM adding a fixed-timestep accumulator in App::run() and a per-frame
REM camera + 1.3x match zoom in App::render(). This test guards the
REM contract that the simulation continues to run at exactly
REM config::kTargetFps (60 Hz) regardless of how fast the desktop
REM spins the render loop.
REM
REM The P17 test does NOT need a display, audio device, or any of
REM EmberClash's .cpp.obj files - the assertions are pure Clock +
REM Settings + Config constant checks. The runner therefore uses a
REM minimal compile + link line (no SDL3.lib, no .obj files) and a
REM Release-mode /MD runtime.
REM
REM Prerequisites:
REM   1. The main game has been configured once so the SDL3 headers
REM      exist under build/_deps/sdl3-src/include :
REM          cmake -S . -B build -G Ninja
REM   2. cl.exe is on PATH (x64 Native Tools prompt, or after
REM      vcvars64.bat).
REM ==========================================================================
setlocal
set "REPO=%~dp0.."
set "BUILD=%~dp0..\build"
set "TESTS=%~dp0"

set "INCLUDES=/I"%REPO%\src" /I"%BUILD%\_deps\sdl3-src\include" /I"%BUILD%\_deps\sdl3-build\include-revision""

if not exist "%BUILD%\obj_test" mkdir "%BUILD%\obj_test"

cl /nologo /EHsc /std:c++latest /W4 /permissive- /utf-8 /MDd %INCLUDES% /Fo"%BUILD%\obj_test\ember_test_p17.obj" /Fe"%BUILD%\ember_test_p17.exe" "%TESTS%\ember_test_p17_playability.cpp" "%BUILD%\CMakeFiles\EmberClash.dir\src\Core\Time.cpp.obj" "%BUILD%\_deps\sdl3-build\SDL3.lib" /link

if errorlevel 1 (
    echo BUILD FAILED - see above
    exit /b 1
)

copy /Y "%BUILD%\ember_test_p17.exe" "%BUILD%\bin\ember_test_p17.exe" >nul
"%BUILD%\bin\ember_test_p17.exe"
exit /b %ERRORLEVEL%
