@echo off
call "%~dp0set_env.bat"
call "%DELTA_VS_DEVCMD%" -arch=amd64 >nul 2>&1
cd /d "%DELTA_PROJECT_ROOT%"
cmake --preset x64-debug
cmake --build Build/x64-Debug --target DeltaEditor
pause
