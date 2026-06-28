@echo off
call "%~dp0DeltaCmd.bat" test delta-cmd %*
exit /b %ERRORLEVEL%
