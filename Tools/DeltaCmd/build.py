from __future__ import annotations

import sys
from pathlib import Path

from env import EnvContext
from registry import RegistryError, get_preset, resolve_build_target


def run(target_alias: str, preset_name: str, env: EnvContext) -> int:
    try:
        preset = get_preset(preset_name)
        cmake_target = resolve_build_target(target_alias)
    except RegistryError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    binary_dir = preset["binary_dir"]
    if not isinstance(binary_dir, Path):
        print("ERROR: Invalid preset binary_dir.", file=sys.stderr)
        return 1

    command = f"cmake --build {binary_dir.as_posix()} --target {cmake_target}"
    return env.run_vs_command(command)
