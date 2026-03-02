@echo off
setlocal enabledelayedexpansion
REM Set DELTA_PROJECT_ROOT (absolute path to repo root). Script lives in Tools\Scripts\ -> go up two levels.
pushd "%~dp0..\.."
set "DELTA_PROJECT_ROOT=%CD%"
popd

set "DELTA_PYTHON=%DELTA_PROJECT_ROOT%\Tools\Python\python.exe"

REM DELTA_VS_DEVCMD: VsDevCmd.bat path. Prefer VSINSTALLDIR, then vswhere, then hardcoded fallback.
set "DELTA_VS_DEVCMD="
if defined VSINSTALLDIR (
  set "DELTA_VS_DEVCMD=%VSINSTALLDIR%Common7\Tools\VsDevCmd.bat"
  if exist "!DELTA_VS_DEVCMD!" goto :vs_done
)
set "DELTA_VS_DEVCMD="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
  for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -property installationPath`) do (
    set "DELTA_VS_DEVCMD=%%i\Common7\Tools\VsDevCmd.bat"
    if exist "!DELTA_VS_DEVCMD!" goto :vs_done
  )
)
set "DELTA_VS_DEVCMD=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
:vs_done
endlocal & set "DELTA_PROJECT_ROOT=%DELTA_PROJECT_ROOT%" & set "DELTA_PYTHON=%DELTA_PYTHON%" & set "DELTA_VS_DEVCMD=%DELTA_VS_DEVCMD%"
