@echo off
REM ==========================================================================
REM Ember Clash M7-AI polish headless test runner (Windows).
REM
REM Same shape as the M4 / M5 / M6 runners: load vcvars64, build a
REM standalone exe from the test source, then run it. Output goes to
REM the parent console.
REM
REM Prerequisite: tests\verify_desktop_build.bat has been run so the
REM .obj files for the main game exist under build\CMakeFiles\.
REM ==========================================================================
setlocal
set "REPO=%~dp0.."
set "BUILD=%~dp0..\build"
set "TESTS=%~dp0"

set "INCLUDES=/I"%REPO%\src" /I"%BUILD%\_deps\sdl3-src\include" /I"%BUILD%\_deps\sdl3-build\include-revision""

set "OBJS=%BUILD%\CMakeFiles\EmberClash.dir\src\Core\App.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Core\Logger.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Core\Time.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Attack.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\AiController.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Animation.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Fighter.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Sprite.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\InputMap.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Match.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Graphics\Renderer.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Graphics\Window.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Input\InputManager.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Physics\Motion.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\UI\BitmapFont.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\UI\Hud.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\World\Arena.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Audio\Audio.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\UI\Menu.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Core\Settings.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Input\Gamepad.cpp.obj"

set "SDL3=%BUILD%\_deps\sdl3-build\SDL3.lib"

if not exist "%BUILD%\obj_test" mkdir "%BUILD%\obj_test"

cl /nologo /EHsc /std:c++latest /W4 /permissive- /utf-8 /Zi /MDd %INCLUDES% /Fo"%BUILD%\obj_test\ember_test_m7.obj" /Fe"%BUILD%\ember_test_m7.exe" "%TESTS%\ember_test_m7.cpp" /link %OBJS% %SDL3%

if errorlevel 1 (
    echo BUILD FAILED - see above
    exit /b 1
)

copy /Y "%BUILD%\ember_test_m7.exe" "%BUILD%\bin\ember_test_m7.exe" >nul
"%BUILD%\bin\ember_test_m7.exe"
exit /b %ERRORLEVEL%
