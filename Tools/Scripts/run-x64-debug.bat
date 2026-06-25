@echo off
call "%~dp0DeltaCmd.bat" run editor %*
exit /b %ERRORLEVEL%
