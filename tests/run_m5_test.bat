@echo off
REM ==========================================================================
REM Ember Clash M5 headless test runner (Windows).
REM
REM Companion to tests/run_ai_test.bat (M4). Builds and runs tests/
REM ember_test_m5.cpp using the same MSVC cl.exe + SDL3.lib approach so
REM the harness doesn't need its own CMake project.
REM
REM Prerequisites:
REM   1. Main game has been built once so the per-file *.cpp.obj files
REM      exist under build/CMakeFiles/EmberClash.dir/src/... :
REM          cmake -S . -B build -G Ninja
REM          cmake --build build
REM   2. cl.exe is on PATH (x64 Native Tools prompt, or after vcvars64.bat).
REM ==========================================================================
setlocal
set "REPO=%~dp0.."
set "BUILD=%~dp0..\build"
set "TESTS=%~dp0"

set "INCLUDES=/I"%REPO%\src" /I"%BUILD%\_deps\sdl3-src\include" /I"%BUILD%\_deps\sdl3-build\include-revision""

set "OBJS=%BUILD%\CMakeFiles\EmberClash.dir\src\Core\App.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Core\Logger.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Core\Time.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Attack.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\AiController.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Fighter.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\InputMap.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Match.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Graphics\Renderer.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Graphics\Window.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Input\InputManager.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Physics\Motion.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\UI\BitmapFont.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\UI\Hud.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\World\Arena.cpp.obj"

set "SDL3=%BUILD%\_deps\sdl3-build\SDL3.lib"

if not exist "%BUILD%\obj_test" mkdir "%BUILD%\obj_test"

cl /nologo /EHsc /std:c++latest /W4 /permissive- /utf-8 /Zi /MDd %INCLUDES% /Fo"%BUILD%\obj_test\ember_test_m5.obj" /Fe"%BUILD%\ember_test_m5.exe" "%TESTS%\ember_test_m5.cpp" /link %OBJS% %SDL3%

if errorlevel 1 (
    echo BUILD FAILED - see above
    exit /b 1
)

copy /Y "%BUILD%\ember_test_m5.exe" "%BUILD%\bin\ember_test_m5.exe" >nul
"%BUILD%\bin\ember_test_m5.exe"
exit /b %ERRORLEVEL%
