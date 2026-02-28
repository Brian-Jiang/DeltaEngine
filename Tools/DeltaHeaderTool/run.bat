@echo off
setlocal
set "SCRIPT_DIR=%~dp0"
set "ROOT=%SCRIPT_DIR%..\.."
set "PYTHON=%ROOT%\Tools\Python\python.exe"
set "MAIN=%SCRIPT_DIR%main.py"
if not exist "%PYTHON%" (
    echo ERROR: Embedded Python not found at %PYTHON%
    echo Run Tools/Scripts/setup_tools.py with your system Python first.
    exit /b 1
)
cd /d "%ROOT%"
"%PYTHON%" "%MAIN%" %*
