@echo off
setlocal enableextensions

REM ============================================================================
REM M6-C test runner (Windows, MSVC + Ninja, /MDd, loads vcvars64).
REM
REM The M6-C harness is the playability integration test - it drives the
REM real Match through the real InputMap with synthetic key events. It uses
REM the same OBJS list as run_m5_test.bat because it links against the
REM same gameplay modules.
REM ============================================================================

set "REPO=%~dp0.."
set "BUILD=%REPO%\build"
set "TESTS=%REPO%\tests"

if not exist "%BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Sprite.cpp.obj" (
    echo [M6-C] build tree missing - run tests\verify_desktop_build.bat first.
    exit /b 1
)

set "INCLUDES=/I"%REPO%\src" /I"%BUILD%\_deps\sdl3-src\include" /I"%BUILD%\_deps\sdl3-build\include-revision""

set "OBJS=%BUILD%\CMakeFiles\EmberClash.dir\src\Core\App.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Core\Logger.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Core\Time.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Attack.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\AiController.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Animation.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Fighter.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Sprite.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\InputMap.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Match.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Graphics\Renderer.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Graphics\Window.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Input\InputManager.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Physics\Motion.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\UI\BitmapFont.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\UI\Hud.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\World\Arena.cpp.obj"

set "SDL3=%BUILD%\_deps\sdl3-build\SDL3.lib"

if not exist "%BUILD%\obj_test" mkdir "%BUILD%\obj_test"

cl /nologo /EHsc /std:c++latest /W4 /permissive- /utf-8 /Zi /MDd %INCLUDES% /Fo"%BUILD%\obj_test\ember_test_m6c.obj" /Fe"%BUILD%\ember_test_m6c.exe" "%TESTS%\ember_test_m6c.cpp" /link %OBJS% %SDL3%
if errorlevel 1 (
    echo [M6-C] cl failed.
    exit /b 1
)

echo --- M6-C harness running ---
"%BUILD%\ember_test_m6c.exe"
set "RC=%errorlevel%"
echo --- M6-C harness exit %RC% ---
exit /b %RC%
