@echo off
REM Wrapper that loads vcvars64 and then calls the real test script.
REM Used by PowerShell / bash to avoid the (x86) paren being re-parsed
REM by the shell. Mirrors verify_desktop_build.bat's pattern.
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
  echo Failed to load vcvars64
  exit /b 1
)
pushd "%~dp0.."
call "%~dp0run_m6_test.bat"
set _rc=%errorlevel%
popd
exit /b %_rc%
