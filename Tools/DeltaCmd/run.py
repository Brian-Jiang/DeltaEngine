from __future__ import annotations

import sys

from env import EnvContext
from registry import (
    RegistryError,
    executable_path,
    missing_run_hint,
    resolve_run_target,
)


def run(target_alias: str, preset_name: str, extra_args: list[str], env: EnvContext) -> int:
    try:
        exe_name = resolve_run_target(target_alias)
        exe_path = env.project_root / executable_path(preset_name, exe_name)
    except RegistryError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    if not exe_path.is_file():
        print(f"{exe_name} not found at:")
        print(f"  {exe_path}")
        print(missing_run_hint(target_alias))
        return 1

    return env.run_command([exe_path, *extra_args])
