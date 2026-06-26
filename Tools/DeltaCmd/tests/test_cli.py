from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import pytest

import cli
from env import EnvContext


@dataclass(frozen=True)
class FakeEnv:
    project_root: Path
    vs_devcmd: Path
    bundled_python: Path

    def run_vs_command(self, command: str) -> int:
        return 0

    def run_command(self, argv: list[str | Path]) -> int:
        return 0

    def run_gui_command(self, exe_path: Path, extra_args: list[str]) -> int:
        return 0


def _fake_env() -> EnvContext:
    root = Path(__file__).resolve().parents[3]
    return EnvContext(
        project_root=root,
        vs_devcmd=Path("VsDevCmd.bat"),
        bundled_python=root / "Tools" / "Python" / "python.exe",
    )


def test_parser_accepts_known_commands():
    parser = cli._build_parser()
    args = parser.parse_args(["configure"])
    assert args.command == "configure"
    assert args.preset == "x64-debug"

    args = parser.parse_args(["build", "editor"])
    assert args.command == "build"
    assert args.target == "editor"

    args = parser.parse_args(["list"])
    assert args.command == "list"


def test_parser_rejects_unknown_build_target():
    parser = cli._build_parser()
    with pytest.raises(SystemExit):
        parser.parse_args(["build", "unknown"])


def test_main_list_skips_env_context(monkeypatch):
    called = {"create_env": False}

    def fake_create_env_context():
        called["create_env"] = True
        return _fake_env()

    monkeypatch.setattr(cli, "create_env_context", fake_create_env_context)
    monkeypatch.setattr(cli, "finalize", lambda code, automatic: code)

    exit_code = cli.main(["list"])
    assert exit_code == 0
    assert called["create_env"] is False


def test_main_configure_uses_env_context(monkeypatch):
    env = _fake_env()
    captured: dict[str, object] = {}

    monkeypatch.setattr(cli, "create_env_context", lambda: env)
    monkeypatch.setattr(cli, "finalize", lambda code, automatic: code)
    monkeypatch.setattr(
        cli.configure,
        "run",
        lambda preset, ctx: captured.update({"preset": preset, "ctx": ctx}) or 0,
    )

    exit_code = cli.main(["configure", "--preset", "x64-debug"])
    assert exit_code == 0
    assert captured["preset"] == "x64-debug"
    assert captured["ctx"] is env


def test_main_test_forwards_pytest_suite(monkeypatch):
    env = _fake_env()
    captured: dict[str, object] = {}

    monkeypatch.setattr(cli, "create_env_context", lambda: env)
    monkeypatch.setattr(cli, "finalize", lambda code, automatic: code)
    monkeypatch.setattr(
        cli.test,
        "run",
        lambda suite, preset, extra_args, ctx: captured.update(
            {"suite": suite, "preset": preset, "extra_args": extra_args, "ctx": ctx}
        )
        or 0,
    )

    exit_code = cli.main(["test", "delta-cmd", "-k", "test_registry"])
    assert exit_code == 0
    assert captured["suite"] == "delta-cmd"
    assert captured["preset"] == "x64-debug"
    assert captured["extra_args"] == ["-k", "test_registry"]


def test_main_strips_automatic_before_dispatch(monkeypatch):
    env = _fake_env()
    captured: dict[str, bool] = {}

    monkeypatch.setattr(cli, "create_env_context", lambda: env)
    monkeypatch.setattr(
        cli,
        "finalize",
        lambda code, automatic: captured.update({"automatic": automatic}) or code,
    )
    monkeypatch.setattr(cli.list_cmd, "run", lambda: 0)

    exit_code = cli.main(["list", "--automatic"])
    assert exit_code == 0
    assert captured["automatic"] is True
