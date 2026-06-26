from __future__ import annotations

import sys
from pathlib import Path

import pytest

from env import find_project_root, strip_automatic


def test_find_project_root_from_tests_dir():
    root = find_project_root(Path(__file__).resolve().parent)
    assert (root / "CMakePresets.json").is_file()
    assert (root / "Engine").is_dir()


def test_find_project_root_from_delta_cmd_dir():
    delta_cmd_dir = Path(__file__).resolve().parent.parent
    root = find_project_root(delta_cmd_dir)
    assert (root / "CMakePresets.json").is_file()


def test_strip_automatic_filters_flag():
    filtered, automatic = strip_automatic(["build", "editor", "--automatic", "--preset", "x64-debug"])
    assert automatic is True
    assert filtered == ["build", "editor", "--preset", "x64-debug"]


def test_strip_automatic_case_insensitive():
    filtered, automatic = strip_automatic(["--AUTOMATIC", "list"])
    assert automatic is True
    assert filtered == ["list"]


def test_strip_automatic_absent():
    filtered, automatic = strip_automatic(["list"])
    assert automatic is False
    assert filtered == ["list"]


def test_require_windows_exits_on_non_windows(monkeypatch):
    monkeypatch.setattr(sys, "platform", "linux")

    from env import require_windows

    with pytest.raises(SystemExit) as exc_info:
        require_windows()
    assert exc_info.value.code == 1
