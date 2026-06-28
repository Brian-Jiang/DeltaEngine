@echo off
call "%~dp0DeltaCmd.bat" test header-tool %*
exit /b %ERRORLEVEL%
