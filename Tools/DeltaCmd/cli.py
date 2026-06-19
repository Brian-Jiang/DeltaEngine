from __future__ import annotations

import argparse
import sys

import build
import configure
from env import create_env_context, finalize, strip_automatic
from registry import BUILD_TARGETS, PRESETS


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="delta",
        description="DeltaEngine build, run, and test CLI.",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    configure_parser = subparsers.add_parser(
        "configure",
        help="Configure the project with CMake presets.",
    )
    configure_parser.add_argument(
        "--preset",
        default="x64-debug",
        choices=sorted(PRESETS),
        help="CMake configure preset (default: x64-debug).",
    )

    build_parser = subparsers.add_parser(
        "build",
        help="Build a CMake target.",
    )
    build_parser.add_argument(
        "target",
        choices=sorted(BUILD_TARGETS),
        help="Build target alias.",
    )
    build_parser.add_argument(
        "--preset",
        default="x64-debug",
        choices=sorted(PRESETS),
        help="CMake configure preset (default: x64-debug).",
    )

    return parser


def main(argv: list[str] | None = None) -> int:
    raw_argv = list(sys.argv[1:] if argv is None else argv)
    filtered_argv, automatic = strip_automatic(raw_argv)

    parser = _build_parser()
    args = parser.parse_args(filtered_argv)

    env = create_env_context()

    if args.command == "configure":
        exit_code = configure.run(args.preset, env)
    elif args.command == "build":
        exit_code = build.run(args.target, args.preset, env)
    else:
        print(f"ERROR: Unknown command '{args.command}'.", file=sys.stderr)
        exit_code = 1

    return finalize(exit_code, automatic)
