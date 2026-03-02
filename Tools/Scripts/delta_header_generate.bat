@echo off
call "%~dp0set_env.bat"
cd /d "%DELTA_PROJECT_ROOT%"
"%DELTA_PYTHON%" "%DELTA_PROJECT_ROOT%\Tools\DeltaHeaderTool\main.py" --input-dir "Engine/Runtime" --output-dir "%DELTA_PROJECT_ROOT%\Intermediate\DeltaHeaderTool\Generated" --manifest "%DELTA_PROJECT_ROOT%\Intermediate\DeltaHeaderTool\generated_sources.cmake" --engine-root "%DELTA_PROJECT_ROOT%" --include-dir "%DELTA_PROJECT_ROOT%\Engine" --include-dir "%DELTA_PROJECT_ROOT%\Engine\Runtime" --include-dir "%DELTA_PROJECT_ROOT%\Engine\ThirdParty\DirectXTK12\Inc" --include-dir "%DELTA_PROJECT_ROOT%\Engine\ThirdParty\DirectX-Headers\include\directx"
pause
