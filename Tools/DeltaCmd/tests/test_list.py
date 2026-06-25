from __future__ import annotations

import list_cmd
from registry import (
    BUILD_TARGETS,
    GTEST_SUITES,
    HEADER_ACTIONS,
    PRESETS,
    PYTEST_SUITES,
    RUN_TARGETS,
    format_list_output,
)


def test_list_cmd_run_returns_zero(capsys):
    exit_code = list_cmd.run()
    output = capsys.readouterr().out

    assert exit_code == 0
    assert output.rstrip("\n") == format_list_output()


def test_list_output_has_stable_sections(capsys):
    list_cmd.run()
    output = capsys.readouterr().out

    for heading in (
        "DeltaCmd registry",
        "Presets:",
        "Build targets:",
        "Run targets:",
        "Test suites:",
        "Header commands:",
        "Examples:",
    ):
        assert heading in output

    for alias in PRESETS:
        assert alias in output
    for alias in BUILD_TARGETS:
        assert alias in output
    for alias in RUN_TARGETS:
        assert alias in output
    for alias in GTEST_SUITES:
        assert alias in output
    for alias in PYTEST_SUITES:
        assert alias in output
    for action in HEADER_ACTIONS:
        assert action in output
