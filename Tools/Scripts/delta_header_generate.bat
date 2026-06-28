@echo off
call "%~dp0DeltaCmd.bat" header generate %*
exit /b %ERRORLEVEL%
