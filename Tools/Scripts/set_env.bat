@echo off
REM Set DELTA_PROJECT_ROOT (absolute path to repo root). Script lives in Tools\Scripts\ -> go up two levels.
pushd "%~dp0..\.."
set "DELTA_PROJECT_ROOT=%CD%"
popd

set "DELTA_PYTHON=%DELTA_PROJECT_ROOT%\Tools\Python\python.exe"

REM VS dev environment is activated by DeltaCmd (Tools/DeltaCmd/env.py), not here.
