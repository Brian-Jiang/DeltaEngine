@echo off
call "%~dp0DeltaCmd.bat" configure %*
exit /b %ERRORLEVEL%
