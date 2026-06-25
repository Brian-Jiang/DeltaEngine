@echo off
call "%~dp0DeltaCmd.bat" test engine %*
exit /b %ERRORLEVEL%
