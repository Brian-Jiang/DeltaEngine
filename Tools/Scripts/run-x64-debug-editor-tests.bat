@echo off
call "%~dp0set_env.bat"
set "DELTA_TEST_EXE=%DELTA_PROJECT_ROOT%\Build\x64-Debug\bin\DeltaEditorTests.exe"
if not exist "%DELTA_TEST_EXE%" (
  echo DeltaEditorTests.exe not found at:
  echo   %DELTA_TEST_EXE%
  echo Run build-x64-debug-editor-tests.bat first.
  if /i not "%~1"=="--automatic" pause
  exit /b 1
)
cd /d "%DELTA_PROJECT_ROOT%"
"%DELTA_TEST_EXE%" %2 %3 %4 %5 %6 %7 %8 %9
set "EXITCODE=%ERRORLEVEL%"
if /i not "%~1"=="--automatic" pause
exit /b %EXITCODE%
