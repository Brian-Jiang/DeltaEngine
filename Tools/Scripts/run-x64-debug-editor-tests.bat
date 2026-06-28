@echo off
call "%~dp0DeltaCmd.bat" test editor %*
exit /b %ERRORLEVEL%
