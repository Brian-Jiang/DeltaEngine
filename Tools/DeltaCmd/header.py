from __future__ import annotations

import sys

from env import EnvContext
from registry import HEADER_ACTIONS, HEADER_TOOL_SCRIPT, RegistryError, header_tool_argv


def run(action: str, env: EnvContext) -> int:
    if action not in HEADER_ACTIONS:
        print(f"ERROR: Unknown header action '{action}'.", file=sys.stderr)
        return 1

    if not env.bundled_python.is_file():
        print(f"ERROR: Embedded Python not found at {env.bundled_python}", file=sys.stderr)
        return 1

    main_script = env.project_root / HEADER_TOOL_SCRIPT
    if not main_script.is_file():
        print(f"ERROR: DeltaHeaderTool not found at {main_script}", file=sys.stderr)
        return 1

    try:
        tool_args = header_tool_argv(env.project_root, force=(action == "force"))
    except RegistryError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    return env.run_command([env.bundled_python, main_script, *tool_args])
