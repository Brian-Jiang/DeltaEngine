from __future__ import annotations

# VS dev environment discovery and activation live here (Phase 3+).
# set_env.bat only resolves DELTA_PROJECT_ROOT and DELTA_PYTHON.

import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


def _delta_cmd_logging():
    import importlib.util

    module_name = "delta_cmd_logging"
    if module_name in sys.modules:
        return sys.modules[module_name]

    module_path = Path(__file__).resolve().parent / "logging.py"
    spec = importlib.util.spec_from_file_location(module_name, module_path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


@dataclass(frozen=True)
class EnvContext:
    project_root: Path
    vs_devcmd: Path
    bundled_python: Path
    session: object | None = None

    def run_vs_command(self, command: str) -> int:
        if os.environ.get("VSCMD_ARG_TGT_ARCH"):
            wrapped = command
        else:
            vs_devcmd = str(self.vs_devcmd).replace('"', '""')
            wrapped = f'call "{vs_devcmd}" -arch=amd64 >nul 2>&1 && {command}'

        argv = ["cmd", "/c", wrapped]
        if self.session is not None:
            return _delta_cmd_logging().run_subprocess_with_tee(
                argv, cwd=self.project_root, session=self.session
            )

        result = subprocess.run(
            argv,
            cwd=self.project_root,
        )
        return result.returncode

    def run_command(self, argv: list[str | Path]) -> int:
        command = [str(arg) for arg in argv]
        if self.session is not None:
            return _delta_cmd_logging().run_subprocess_with_tee(
                command, cwd=self.project_root, session=self.session
            )

        result = subprocess.run(
            command,
            cwd=self.project_root,
        )
        return result.returncode

    def run_gui_command(self, exe_path: Path, extra_args: list[str]) -> int:
        if self.session is not None:
            self.session.log_meta(f"Launching GUI: {exe_path}")

        exe = str(exe_path).replace('"', '""')
        quoted_args = " ".join(f'"{arg.replace(chr(34), chr(34) + chr(34))}"' for arg in extra_args)
        command = f'start "" /wait "{exe}"'
        if quoted_args:
            command = f"{command} {quoted_args}"
        result = subprocess.run(
            ["cmd", "/c", command],
            cwd=self.project_root,
        )
        return result.returncode


def require_windows() -> None:
    if sys.platform != "win32":
        print("ERROR: DeltaCmd requires Windows (MSVC + VsDevCmd).", file=sys.stderr)
        sys.exit(1)


def _check_windows() -> bool:
    if sys.platform != "win32":
        print("ERROR: DeltaCmd requires Windows (MSVC + VsDevCmd).", file=sys.stderr)
        return False
    return True


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


def try_find_vs_devcmd() -> Path | None:
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
    return None


def strip_automatic(argv: list[str]) -> tuple[list[str], bool]:
    automatic = False
    filtered: list[str] = []
    for arg in argv:
        if arg.lower() == "--automatic":
            automatic = True
        else:
            filtered.append(arg)
    return filtered, automatic


def create_env_context(
    project_root: Path | None = None,
    session: object | None = None,
) -> EnvContext | None:
    if not _check_windows():
        return None

    root = project_root if project_root is not None else find_project_root()
    vs_devcmd = try_find_vs_devcmd()
    if vs_devcmd is None:
        return None

    bundled_python = root / "Tools" / "Python" / "python.exe"
    return EnvContext(
        project_root=root,
        vs_devcmd=vs_devcmd,
        bundled_python=bundled_python,
        session=session,
    )


def pause_if_interactive(automatic: bool) -> None:
    if not automatic:
        input("Press any key to continue...")


def finalize(exit_code: int, automatic: bool) -> int:
    pause_if_interactive(automatic)
    return exit_code
