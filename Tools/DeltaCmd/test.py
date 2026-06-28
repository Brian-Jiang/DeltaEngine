from __future__ import annotations

import sys

from env import EnvContext
from registry import (
    RegistryError,
    executable_path,
    missing_test_hint,
    resolve_pytest_suite,
    resolve_test_suite,
)


def normalize_extra_args(args: list[str]) -> list[str]:
    if args and args[0] == "--":
        return args[1:]
    return args


def run(suite: str, preset_name: str, extra_args: list[str], env: EnvContext) -> int:
    extra_args = normalize_extra_args(extra_args)

    try:
        exe_name = resolve_test_suite(suite)
    except RegistryError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    if exe_name is None:
        if not env.bundled_python.is_file():
            print(f"ERROR: Embedded Python not found at {env.bundled_python}", file=sys.stderr)
            return 1

        try:
            test_dir = resolve_pytest_suite(suite)
        except RegistryError as exc:
            print(f"ERROR: {exc}", file=sys.stderr)
            return 1

        return env.run_command(
            [
                env.bundled_python,
                "-m",
                "pytest",
                str(test_dir),
                "-v",
                *extra_args,
            ]
        )

    exe_path = env.project_root / executable_path(preset_name, exe_name)
    if not exe_path.is_file():
        print(f"{exe_name} not found at:")
        print(f"  {exe_path}")
        print(missing_test_hint(suite))
        return 1

    return env.run_command([exe_path, *extra_args])
