# Run DeltaHeaderTool using the embedded Python at Tools/Python/
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$Root = (Resolve-Path (Join-Path $ScriptDir "..\..")).Path
$Python = Join-Path $Root "Tools\Python\python.exe"
$Main = Join-Path $ScriptDir "main.py"
if (-not (Test-Path $Python)) {
    Write-Error "Embedded Python not found at $Python. Run Tools/Scripts/setup_tools.py with your system Python first."
    exit 1
}
Set-Location $Root
& $Python $Main $args
