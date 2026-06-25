@echo off
call "%~dp0DeltaCmd.bat" build engine-tests %*
exit /b %ERRORLEVEL%
