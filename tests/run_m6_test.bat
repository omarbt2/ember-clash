@echo off
REM ==========================================================================
REM Ember Clash M6 headless test runner (Windows).
REM
REM Builds and runs tests/ember_test_m6.cpp (the M6-A art-identity / Sprite
REM harness) using the same MSVC cl.exe + SDL3.lib approach as the M4 / M5
REM runners. No SDL init, no display. The M6-A test only touches ArtConfig
REM (header-only) and Sprite, so the OBJS list is the minimal subset that
REM resolves Sprite's ctor symbol.
REM
REM Prerequisites:
REM   1. Main game has been built once (Debug config) so Sprite.cpp.obj
REM      exists under build/CMakeFiles/EmberClash.dir/src/Gameplay/.
REM   2. cl.exe is on PATH (x64 Native Tools prompt, or after vcvars64.bat).
REM ==========================================================================
setlocal
set "REPO=%~dp0.."
set "BUILD=%~dp0..\build"
set "TESTS=%~dp0"

set "INCLUDES=/I"%REPO%\src" /I"%BUILD%\_deps\sdl3-src\include" /I"%BUILD%\_deps\sdl3-build\include-revision""

set "OBJS=%BUILD%\CMakeFiles\EmberClash.dir\src\Gameplay\Sprite.cpp.obj"

set "SDL3=%BUILD%\_deps\sdl3-build\SDL3.lib"

if not exist "%BUILD%\obj_test" mkdir "%BUILD%\obj_test"

cl /nologo /EHsc /std:c++latest /W4 /permissive- /utf-8 /Zi /MDd %INCLUDES% /Fo"%BUILD%\obj_test\ember_test_m6.obj" /Fe"%BUILD%\ember_test_m6.exe" "%TESTS%\ember_test_m6.cpp" /link %OBJS% %SDL3%

if errorlevel 1 (
    echo BUILD FAILED - see above
    exit /b 1
)

copy /Y "%BUILD%\ember_test_m6.exe" "%BUILD%\bin\ember_test_m6.exe" >nul
"%BUILD%\bin\ember_test_m6.exe"
exit /b %ERRORLEVEL%
