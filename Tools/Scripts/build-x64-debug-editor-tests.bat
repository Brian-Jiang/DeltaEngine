@echo off
call "%~dp0DeltaCmd.bat" build editor-tests %*
exit /b %ERRORLEVEL%
