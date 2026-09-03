@echo off
REM ==========================================================================
REM Ember Clash M12.5-AUDIO headless test runner (Windows).
REM
REM Tests the real SDL3 audio backend: procedural bank generation,
REM mute gate, volume gate, headless fallback, bank injection, and
REM shutdown safety. Same build / link pattern as run_m12_test.bat.
REM ==========================================================================
setlocal EnableExtensions
set "ROOT=%~dp0.."
set "BUILD=%ROOT%\build"
set "TESTS=%ROOT%\tests"

set "INCLUDES=/I"%ROOT%\src" /I"%BUILD%\_deps\sdl3-src\include" /I"%BUILD%\_deps\sdl3-build\include-revision""
set "SDL3=%BUILD%\_deps\sdl3-build\SDL3.lib"

if not exist "%BUILD%\CMakeFiles\EmberClash.dir\src\World\Arena.cpp.obj" (
  echo M12.5-AUDIO: run cmake --build first
  exit /b 1
)

set "OBJS=%BUILD%\CMakeFiles\EmberClash.dir\src\Core\App.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Core\Logger.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Core\Time.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Attack.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\AiController.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Animation.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Fighter.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Sprite.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\InputMap.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Match.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Audio\Audio.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Graphics\Renderer.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Graphics\Window.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Input\InputManager.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Input\Gamepad.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Physics\Motion.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\UI\BitmapFont.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\UI\Hud.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\UI\Menu.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\World\Arena.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Core\Settings.cpp.obj"

cl /nologo /EHsc /std:c++latest /W4 /permissive- /utf-8 /Zi /MDd %INCLUDES% /Fo"%BUILD%\obj_test\ember_test_m125_audio.obj" /Fe"%BUILD%\ember_test_m125_audio.exe" "%TESTS%\ember_test_m125_audio.cpp" /link %OBJS% %SDL3% user32.lib
if errorlevel 1 (
  echo [M12.5-AUDIO] cl failed.
  exit /b 1
)

if not exist "%BUILD%\SDL3.dll" (
  copy /Y "%BUILD%\_deps\sdl3-build\SDL3.dll" "%BUILD%\SDL3.dll" >nul
)

"%BUILD%\ember_test_m125_audio.exe"
set "_rc=%errorlevel%"
exit /b %_rc%
