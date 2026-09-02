@echo off
REM ==========================================================================
REM Ember Clash M4 headless AI test runner (Windows).
REM
REM This script is checked in to tests/ so the test source and its runner
REM live together. A build-time copy of this file is also placed at
REM build/run_ai_test.bat by M4 (it lives under build/ because it needs
REM the per-file *.cpp.obj files that CMake produces there).
REM
REM Prerequisites:
REM   1. The main game has been built at least once so the .obj files
REM      exist under build/CMakeFiles/EmberClash.dir/src/... :
REM          cmake -S . -B build -G Ninja
REM          cmake --build build
REM   2. cl.exe is on PATH (run from a "x64 Native Tools" prompt, or
REM      after calling vcvars64.bat manually).
REM ==========================================================================
setlocal
set "REPO=%~dp0.."
set "BUILD=%~dp0..\build"
set "TESTS=%~dp0"

set "INCLUDES=/I"%REPO%\src" /I"%BUILD%\_deps\sdl3-src\include" /I"%BUILD%\_deps\sdl3-build\include-revision""

set "OBJS=%BUILD%\CMakeFiles\EmberClash.dir\src\Core\App.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Core\Logger.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Core\Time.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Attack.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\AiController.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Fighter.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\InputMap.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Match.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Graphics\Renderer.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Graphics\Window.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Input\InputManager.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Physics\Motion.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\World\Arena.cpp.obj"

set "SDL3=%BUILD%\_deps\sdl3-build\SDL3.lib"

if not exist "%BUILD%\obj_test" mkdir "%BUILD%\obj_test"

cl /nologo /EHsc /std:c++latest /W4 /permissive- /utf-8 /Zi /MDd %INCLUDES% /Fo"%BUILD%\obj_test\ember_test_ai.obj" /Fe"%BUILD%\ember_test_ai.exe" "%TESTS%\ember_test_ai.cpp" /link %OBJS% %SDL3%

if errorlevel 1 (
    echo BUILD FAILED - see above
    exit /b 1
)

copy /Y "%BUILD%\ember_test_ai.exe" "%BUILD%\bin\ember_test_ai.exe" >nul
"%BUILD%\bin\ember_test_ai.exe"
exit /b %ERRORLEVEL%
