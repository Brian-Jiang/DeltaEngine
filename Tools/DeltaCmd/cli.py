from __future__ import annotations

import argparse
import sys

import build
import configure
import header
import run as run_cmd
import test
from env import create_env_context, finalize, strip_automatic
from registry import BUILD_TARGETS, PRESETS, RUN_TARGETS, TEST_SUITES


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

    run_parser = subparsers.add_parser(
        "run",
        help="Launch a built executable.",
    )
    run_parser.add_argument(
        "target",
        choices=sorted(RUN_TARGETS),
        help="Run target alias.",
    )
    run_parser.add_argument(
        "--preset",
        default="x64-debug",
        choices=sorted(PRESETS),
        help="CMake configure preset (default: x64-debug).",
    )
    run_parser.add_argument(
        "extra_args",
        nargs=argparse.REMAINDER,
        help="Extra arguments forwarded to the executable.",
    )

    test_parser = subparsers.add_parser(
        "test",
        help="Run a test suite.",
    )
    test_parser.add_argument(
        "suite",
        choices=sorted(TEST_SUITES),
        help="Test suite alias.",
    )
    test_parser.add_argument(
        "--preset",
        default="x64-debug",
        choices=sorted(PRESETS),
        help="CMake configure preset (default: x64-debug).",
    )
    test_parser.add_argument(
        "extra_args",
        nargs=argparse.REMAINDER,
        help="Extra arguments forwarded to the test runner.",
    )

    header_parser = subparsers.add_parser(
        "header",
        help="Run DeltaHeaderTool reflection codegen.",
    )
    header_subparsers = header_parser.add_subparsers(dest="header_action")
    header_subparsers.add_parser(
        "generate",
        help="Incremental reflection codegen (default).",
    )
    header_subparsers.add_parser(
        "force",
        help="Force full reflection codegen.",
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
    elif args.command == "run":
        exit_code = run_cmd.run(
            args.target,
            args.preset,
            test.normalize_extra_args(args.extra_args),
            env,
        )
    elif args.command == "test":
        exit_code = test.run(
            args.suite,
            args.preset,
            test.normalize_extra_args(args.extra_args),
            env,
        )
    elif args.command == "header":
        exit_code = header.run(args.header_action or "generate", env)
    else:
        print(f"ERROR: Unknown command '{args.command}'.", file=sys.stderr)
        exit_code = 1

    return finalize(exit_code, automatic)
