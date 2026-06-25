from __future__ import annotations

import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class EnvContext:
    project_root: Path
    vs_devcmd: Path
    bundled_python: Path

    def run_vs_command(self, command: str) -> int:
        vs_devcmd = str(self.vs_devcmd).replace('"', '""')
        wrapped = f'call "{vs_devcmd}" -arch=amd64 >nul 2>&1 && {command}'
        result = subprocess.run(
            ["cmd", "/c", wrapped],
            cwd=self.project_root,
        )
        return result.returncode

    def run_command(self, argv: list[str | Path]) -> int:
        command = [str(arg) for arg in argv]
        result = subprocess.run(
            command,
            cwd=self.project_root,
        )
        return result.returncode


def require_windows() -> None:
    if sys.platform != "win32":
        print("ERROR: DeltaCmd requires Windows (MSVC + VsDevCmd).", file=sys.stderr)
        sys.exit(1)


def find_project_root(start: Path | None = None) -> Path:
    candidates: list[Path] = []
    if start is not None:
        candidates.append(start.resolve())
    candidates.append(Path(__file__).resolve().parent)
    candidates.append(Path.cwd())

    seen: set[Path] = set()
    for origin in candidates:
        current = origin
        if current.is_file():
            current = current.parent
        while True:
            resolved = current.resolve()
            if resolved not in seen:
                seen.add(resolved)
                if (resolved / "CMakePresets.json").is_file() and (resolved / "Engine").is_dir():
                    return resolved
            if current.parent == current:
                break
            current = current.parent

    print(
        "ERROR: Could not find DeltaEngine project root "
        "(expected CMakePresets.json and Engine/).",
        file=sys.stderr,
    )
    sys.exit(1)


def find_vs_devcmd() -> Path:
    vs_install_dir = os.environ.get("VSINSTALLDIR")
    if vs_install_dir:
        candidate = Path(vs_install_dir) / "Common7" / "Tools" / "VsDevCmd.bat"
        if candidate.is_file():
            return candidate

    program_files_x86 = os.environ.get("ProgramFiles(x86)")
    if program_files_x86:
        vswhere = Path(program_files_x86) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
        if vswhere.is_file():
            result = subprocess.run(
                [str(vswhere), "-latest", "-property", "installationPath"],
                capture_output=True,
                text=True,
                check=False,
            )
            installation_path = result.stdout.strip()
            if result.returncode == 0 and installation_path:
                candidate = Path(installation_path) / "Common7" / "Tools" / "VsDevCmd.bat"
                if candidate.is_file():
                    return candidate

    fallback = Path(
        r"C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat"
    )
    if fallback.is_file():
        return fallback

    print("ERROR: VsDevCmd.bat not found.", file=sys.stderr)
    sys.exit(1)


def strip_automatic(argv: list[str]) -> tuple[list[str], bool]:
    automatic = False
    filtered: list[str] = []
    for arg in argv:
        if arg.lower() == "--automatic":
            automatic = True
        else:
            filtered.append(arg)
    return filtered, automatic


def create_env_context() -> EnvContext:
    require_windows()
    project_root = find_project_root()
    vs_devcmd = find_vs_devcmd()
    bundled_python = project_root / "Tools" / "Python" / "python.exe"
    return EnvContext(
        project_root=project_root,
        vs_devcmd=vs_devcmd,
        bundled_python=bundled_python,
    )


def pause_if_interactive(automatic: bool) -> None:
    if not automatic:
        input("Press any key to continue...")


def finalize(exit_code: int, automatic: bool) -> int:
    pause_if_interactive(automatic)
    return exit_code
