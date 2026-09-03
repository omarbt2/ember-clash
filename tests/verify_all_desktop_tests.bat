@echo off
REM Build the desktop game and run all three headless test scripts in one
REM vcvars session, so PowerShell does not need to re-parse the path with
REM embedded parens. Used by M8+ audits to confirm the M4/M5/M8 test
REM surface still passes after a web build was added.
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
  echo Failed to load vcvars64
  exit /b 1
)
pushd "%~dp0.."
cmake --build build --config Debug
if errorlevel 1 (
  echo MAIN BUILD FAILED
  popd
  exit /b 1
)
call "%~dp0run_ai_test.bat"
set _ai=%errorlevel%
call "%~dp0run_m5_test.bat"
set _m5=%errorlevel%
call "%~dp0run_m8_test.bat"
set _m8=%errorlevel%
popd
echo ===summary===
echo AI: %_ai%
echo M5: %_m5%
echo M8: %_m8%
if %_ai% neq 0 exit /b %_ai%
if %_m5% neq 0 exit /b %_m5%
if %_m8% neq 0 exit /b %_m8%
exit /b 0
