from __future__ import annotations

import argparse
import sys

import build
import configure
import header
import list_cmd
import run as run_cmd
import test
from env import create_env_context, finalize, strip_automatic
from registry import BUILD_TARGETS, FORWARDER, PRESETS, RUN_TARGETS, TEST_SUITES

_FORWARDER = FORWARDER.replace("/", "\\")

_ROOT_EPILOG = f"""\
commands:
  configure   Configure the project with CMake presets
  build       Build a CMake target
  run         Launch a built executable
  test        Run a GTest or pytest suite
  header      Run DeltaHeaderTool reflection codegen
  list        Print presets, targets, and example commands

Pass --automatic to skip the interactive pause (required for CI and agents).

examples:
  {_FORWARDER} list --automatic
  {_FORWARDER} build editor --automatic
  {_FORWARDER} test engine --automatic -- --gtest_filter=Foo*
"""


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="delta",
        description="DeltaEngine build, run, and test CLI.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=_ROOT_EPILOG,
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    configure_parser = subparsers.add_parser(
        "configure",
        help="Configure the project with CMake presets.",
        description="Run cmake --preset for the selected configure preset.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""\
examples:
  {_FORWARDER} configure --automatic
  {_FORWARDER} configure --preset x64-debug --automatic
""",
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
        description="Run cmake --build for a registered build target alias.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""\
examples:
  {_FORWARDER} build editor --automatic
  {_FORWARDER} build engine-tests --automatic
  {_FORWARDER} build editor-tests --preset x64-debug --automatic
""",
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
        description="Launch a built executable from the preset binary directory.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""\
examples:
  {_FORWARDER} run editor --automatic
  {_FORWARDER} run editor --automatic -- --some-arg
""",
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
        description="Run a GTest executable or a pytest tool suite; extra args are forwarded.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""\
examples:
  {_FORWARDER} test engine --automatic
  {_FORWARDER} test editor --automatic -- --gtest_filter=Foo*
  {_FORWARDER} test header-tool --automatic -k test_parser
  {_FORWARDER} test delta-cmd --automatic
""",
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
        description="Run DeltaHeaderTool for incremental or full reflection codegen.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""\
examples:
  {_FORWARDER} header generate --automatic
  {_FORWARDER} header force --automatic
""",
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

    subparsers.add_parser(
        "list",
        help="Print presets, targets, and example commands.",
        description="Print presets, build/run/test targets, header commands, and example invocations.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""\
examples:
  {_FORWARDER} list --automatic
""",
    )

    return parser


def main(argv: list[str] | None = None) -> int:
    raw_argv = list(sys.argv[1:] if argv is None else argv)
    filtered_argv, automatic = strip_automatic(raw_argv)

    parser = _build_parser()
    args = parser.parse_args(filtered_argv)

    if args.command == "list":
        exit_code = list_cmd.run()
        return finalize(exit_code, automatic)

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
