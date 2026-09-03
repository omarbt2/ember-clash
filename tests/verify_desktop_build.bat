@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
  echo Failed to load vcvars64
  exit /b 1
)
pushd "%~dp0.."
cmake --build build --config Debug 2>&1
set _rc=%errorlevel%
popd
exit /b %_rc%
