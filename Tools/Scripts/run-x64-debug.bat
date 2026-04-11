@echo off
call "%~dp0set_env.bat"
set "DELTA_LAUNCH_EXE=%DELTA_PROJECT_ROOT%\Build\x64-Debug\bin\DeltaEditorLaunch.exe"
if not exist "%DELTA_LAUNCH_EXE%" (
  echo DeltaEditorLaunch.exe not found at:
  echo   %DELTA_LAUNCH_EXE%
  echo Run build-x64-debug.bat or rebuild-x64-debug.bat first.
  if /i not "%~1"=="--automatic" pause
  exit /b 1
)
cd /d "%DELTA_PROJECT_ROOT%"
start "" /wait "%DELTA_LAUNCH_EXE%"
set "EXITCODE=%ERRORLEVEL%"
if /i not "%~1"=="--automatic" pause
exit /b %EXITCODE%
