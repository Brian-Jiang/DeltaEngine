@echo off
cd /d "%~dp0..\.."
Tools\Python\python.exe -m pytest Tools\DeltaHeaderTool\tests\ -v %*
pause
