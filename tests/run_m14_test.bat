@echo off
REM ==========================================================================
REM Ember Clash M14 headless test runner (Windows).
REM
REM Tests the gamepad haptics wiring (Gamepad::can_rumble/rumble/stop_rumble
REM and the Haptics routing subsystem). Mirrors the build/link pattern from
REM run_m12_test.bat: uses vcvars and links the same .obj list as the real
REM game (so this harness is a real test of the production code, not a mock).
REM ==========================================================================
setlocal EnableExtensions
set "ROOT=%~dp0.."
set "BUILD=%ROOT%\build"
set "TESTS=%ROOT%\tests"

set "INCLUDES=/I"%ROOT%\src" /I"%BUILD%\_deps\sdl3-src\include" /I"%BUILD%\_deps\sdl3-build\include-revision""
set "SDL3=%BUILD%\_deps\sdl3-build\SDL3.lib"

if not exist "%BUILD%\CMakeFiles\EmberClash.dir\src\World\Arena.cpp.obj" (
  echo M14: run cmake --build first
  exit /b 1
)

set "OBJS=%BUILD%\CMakeFiles\EmberClash.dir\src\Input\Gamepad.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Input\Haptics.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Input\InputManager.cpp.obj %BUILD%\CMakeFiles\EmberClash.dir\src\Core\Logger.cpp.obj"

cl /nologo /EHsc /std:c++latest /W4 /permissive- /utf-8 /Zi /MDd %INCLUDES% /Fo"%BUILD%\obj_test\ember_test_m14.obj" /Fe"%BUILD%\ember_test_m14.exe" "%TESTS%\ember_test_m14.cpp" /link %OBJS% %SDL3% user32.lib
if errorlevel 1 (
  echo [M14] cl failed.
  exit /b 1
)

if not exist "%BUILD%\SDL3.dll" (
  copy /Y "%BUILD%\_deps\sdl3-build\SDL3.dll" "%BUILD%\SDL3.dll" >nul
)

"%BUILD%\ember_test_m14.exe"
set "_rc=%errorlevel%"
exit /b %_rc%
