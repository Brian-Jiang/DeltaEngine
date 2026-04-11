@echo off
call "%~dp0set_env.bat"
call "%DELTA_VS_DEVCMD%" -arch=amd64 >nul 2>&1
cd /d "%DELTA_PROJECT_ROOT%"
cmake --build Build/x64-Debug --target DeltaEditorTests
set "EXITCODE=%ERRORLEVEL%"
if /i not "%~1"=="--automatic" pause
exit /b %EXITCODE%
