@echo off
REM Wrapper that loads vcvars64 then runs the M6-B test runner. PowerShell
REM re-parses the (x86) Program Files path inside the VS env, so we use a
REM .cmd / .bat pair: this .cmd is the one PowerShell calls into, and it
REM delegates to run_m6b_test.bat (which only runs once vcvars64 is loaded).

call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (
    echo [M6-B] failed to load vcvars64 - is VS Build Tools 2022 installed?
    exit /b 1
)
call "%~dp0run_m6b_test.bat"
exit /b %errorlevel%
