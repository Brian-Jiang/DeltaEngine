@echo off
call "%~dp0DeltaCmd.bat" configure %*
if errorlevel 1 exit /b %ERRORLEVEL%
call "%~dp0DeltaCmd.bat" build editor %*
exit /b %ERRORLEVEL%
