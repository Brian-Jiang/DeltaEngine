@echo off
call "%~dp0DeltaCmd.bat" build editor %*
exit /b %ERRORLEVEL%
