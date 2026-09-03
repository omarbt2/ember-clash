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
call "%~dp0run_m6_test.bat"
set _m6=%errorlevel%
call "%~dp0run_m6b_test.bat"
set _m6b=%errorlevel%
call "%~dp0run_m6c_test.bat"
set _m6c=%errorlevel%
call "%~dp0run_m7_test.bat"
set _m7=%errorlevel%
call "%~dp0run_m8_test.bat"
set _m8=%errorlevel%
popd
echo ===summary===
echo AI: %_ai%
echo M5: %_m5%
echo M6: %_m6%
echo M6-B: %_m6b%
echo M6-C: %_m6c%
echo M7-AI: %_m7%
echo M8: %_m8%
if %_ai% neq 0 exit /b %_ai%
if %_m5% neq 0 exit /b %_m5%
if %_m6% neq 0 exit /b %_m6%
if %_m6b% neq 0 exit /b %_m6b%
if %_m6c% neq 0 exit /b %_m6c%
if %_m7% neq 0 exit /b %_m7%
if %_m8% neq 0 exit /b %_m8%
exit /b 0
