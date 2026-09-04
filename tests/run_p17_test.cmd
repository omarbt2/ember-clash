@echo off
REM P17 test runner (cmd variant for systems where .bat is blocked).
call "%~dp0run_p17_test.bat"
exit /b %ERRORLEVEL%
