@echo off
call "%~dp0DeltaCmd.bat" header force %*
exit /b %ERRORLEVEL%
