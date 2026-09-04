@echo off
REM ==========================================================================
REM Ember Clash M12 headless test runner (Windows).
REM
REM Tests the Menu state machine and its integration with the Settings
REM + Audio + Gamepad stack. Mirrors the build/link pattern from
REM run_m6c_test.bat / run_m11_test.bat: uses vcvars and links the
REM same .obj list as the real game (so this harness is a real test of
REM the production code, not a mock).
REM ==========================================================================
setlocal EnableExtensions
set "ROOT=%~dp0.."
set "BUILD=%ROOT%\build"
set "TESTS=%ROOT%\tests"

set "INCLUDES=/I"%ROOT%\src" /I"%BUILD%\_deps\sdl3-src\include" /I"%BUILD%\_deps\sdl3-build\include-revision""
set "SDL3=%BUILD%\_deps\sdl3-build\SDL3.lib"

if not exist "%BUILD%\CMakeFiles\EmberClash.dir\src\World\Arena.cpp.obj" (
  echo M12: run cmake --build first
  exit /b 1
)

set "OBJS=%BUILD%\CMakeFiles\EmberClash.dir\src\Core\App.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Core\Logger.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Core\Time.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Attack.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\AiController.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Animation.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Fighter.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Sprite.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\InputMap.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Match.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Audio\Audio.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Graphics\Renderer.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Graphics\Window.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Input\Haptics.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Input\InputManager.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Input\Gamepad.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Physics\Motion.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\UI\BitmapFont.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\UI\Hud.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\UI\Menu.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\World\Arena.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Core\Settings.cpp.obj"

cl /nologo /EHsc /std:c++latest /W4 /permissive- /utf-8 /Zi /MDd %INCLUDES% /Fo"%BUILD%\obj_test\ember_test_m12.obj" /Fe"%BUILD%\ember_test_m12.exe" "%TESTS%\ember_test_m12.cpp" /link %OBJS% %SDL3% user32.lib
if errorlevel 1 (
  echo [M12] cl failed.
  exit /b 1
)

if not exist "%BUILD%\SDL3.dll" (
  copy /Y "%BUILD%\_deps\sdl3-build\SDL3.dll" "%BUILD%\SDL3.dll" >nul
)

"%BUILD%\ember_test_m12.exe"
set "_rc=%errorlevel%"
exit /b %_rc%
