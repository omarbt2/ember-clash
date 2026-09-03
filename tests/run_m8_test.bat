@echo off
REM ==========================================================================
REM Ember Clash M8 headless test runner (Windows).
REM
REM Companion to tests/run_m5_test.bat (M5) and tests/run_ai_test.bat (M4).
REM The M8 test guards the App::step() / App::request_exit() public surface
REM that the Emscripten entry point relies on. It compiles a tiny test source
REM that just static_asserts on the function signatures of those methods.
REM
REM The M8 test does NOT need any of EmberClash's .cpp.obj files - the
REM static_asserts trigger at compile time, and the runtime path is just
REM `return 0`. The M8 runner therefore uses a minimal compile + link line
REM (no SDL3.lib, no .obj files) and a Release-mode /MD runtime to mirror
REM the type of build the Emscripten target will produce on the web side.
REM
REM Prerequisites:
REM   1. The main game has been configured once so the SDL3 headers exist
REM      under build/_deps/sdl3-src/include :
REM          cmake -S . -B build -G Ninja
REM      (we only need the headers, not a full build)
REM   2. cl.exe is on PATH (x64 Native Tools prompt, or after vcvars64.bat).
REM ==========================================================================
setlocal
set "REPO=%~dp0.."
set "BUILD=%~dp0..\build"
set "TESTS=%~dp0"

set "INCLUDES=/I"%REPO%\src" /I"%BUILD%\_deps\sdl3-src\include" /I"%BUILD%\_deps\sdl3-build\include-revision""

if not exist "%BUILD%\obj_test" mkdir "%BUILD%\obj_test"

REM The M8 test deliberately uses /MD (release runtime) and /O2 to mirror the
REM way the Emscripten target compiles. If App::step() or App::request_exit()
REM ever disappear or change signature, the static_asserts fail the compile
REM with a message pointing at src/main_emscripten.cpp.
cl /nologo /EHsc /std:c++latest /W4 /permissive- /utf-8 /O2 /MD %INCLUDES% /Fo"%BUILD%\obj_test\ember_test_m8.obj" /Fe"%BUILD%\ember_test_m8.exe" "%TESTS%\ember_test_m8.cpp" /link

if errorlevel 1 (
    echo BUILD FAILED - see above
    exit /b 1
)

copy /Y "%BUILD%\ember_test_m8.exe" "%BUILD%\bin\ember_test_m8.exe" >nul
"%BUILD%\bin\ember_test_m8.exe"
exit /b %ERRORLEVEL%
