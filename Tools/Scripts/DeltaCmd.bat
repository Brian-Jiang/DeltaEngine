@echo off
call "%~dp0set_env.bat"
"%DELTA_PYTHON%" "%DELTA_PROJECT_ROOT%\Tools\DeltaCmd\main.py" %*
exit /b %ERRORLEVEL%
