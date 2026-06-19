from __future__ import annotations

import sys

from env import EnvContext
from registry import RegistryError, get_preset


def run(preset_name: str, env: EnvContext) -> int:
    try:
        preset = get_preset(preset_name)
    except RegistryError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    configure_preset = preset["configure_preset"]
    command = f"cmake --preset {configure_preset}"
    return env.run_vs_command(command)
