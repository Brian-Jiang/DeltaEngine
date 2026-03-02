@echo off
call "%~dp0set_env.bat"
set "DELTA_EDITOR_EXE=%DELTA_PROJECT_ROOT%\Build\x64-Debug\bin\DeltaEditor.exe"
if not exist "%DELTA_EDITOR_EXE%" (
  echo DeltaEditor.exe not found at:
  echo   %DELTA_EDITOR_EXE%
  echo Run build-x64-debug.bat or rebuild-x64-debug.bat first.
  pause
  exit /b 1
)
cd /d "%DELTA_PROJECT_ROOT%"
start "" /wait "%DELTA_EDITOR_EXE%"
pause
