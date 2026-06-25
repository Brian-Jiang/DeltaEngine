from __future__ import annotations

from pathlib import Path
from unittest.mock import MagicMock

import test as test_cmd
from registry import PYTEST_SUITES


def test_run_pytest_suite_uses_registry_path():
    env = MagicMock()
    env.project_root = Path("/repo")
    env.bundled_python.is_file.return_value = True

    captured: dict[str, list] = {}

    def fake_run_command(argv):
        captured["argv"] = argv
        return 0

    env.run_command = fake_run_command

    exit_code = test_cmd.run("delta-cmd", "x64-debug", ["-k", "test_registry"], env)
    assert exit_code == 0
    assert captured["argv"][0] == env.bundled_python
    assert captured["argv"][1:4] == ["-m", "pytest", str(PYTEST_SUITES["delta-cmd"])]
    assert captured["argv"][4:] == ["-v", "-k", "test_registry"]
