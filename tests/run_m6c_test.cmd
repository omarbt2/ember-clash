@echo off
REM Wrapper that loads vcvars64 and then calls the real test script.
REM Mirrors verify_desktop_build.bat's pattern. The 2019 BuildTools path
REM is what the rest of the test infrastructure uses.
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
  echo Failed to load vcvars64
  exit /b 1
)
pushd "%~dp0.."
call "%~dp0run_m6c_test.bat"
set _rc=%errorlevel%
popd
exit /b %_rc%
