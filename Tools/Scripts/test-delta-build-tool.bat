@echo off
setlocal enabledelayedexpansion
call "%~dp0set_env.bat"
set "AUTOMATIC=0"
set "PYTEST_ARGS="
:parse
if "%~1"=="" goto :run
if /i "%~1"=="--automatic" (
  set "AUTOMATIC=1"
) else (
  set "PYTEST_ARGS=!PYTEST_ARGS! %~1"
)
shift
goto :parse
:run
cd /d "%DELTA_PROJECT_ROOT%"
"%DELTA_PYTHON%" -m pytest Tools\DeltaBuildTool\tests\ -v !PYTEST_ARGS!
set "EXITCODE=%ERRORLEVEL%"
if "%AUTOMATIC%"=="0" pause
exit /b %EXITCODE%
