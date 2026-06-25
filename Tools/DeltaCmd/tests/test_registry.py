from __future__ import annotations

from pathlib import Path

import pytest

from registry import (
    BUILD_TARGETS,
    GTEST_SUITES,
    HEADER_ACTIONS,
    PRESETS,
    PYTEST_SUITES,
    RUN_TARGETS,
    TEST_SUITES,
    RegistryError,
    executable_path,
    format_list_output,
    get_preset,
    resolve_build_target,
    resolve_pytest_suite,
    resolve_run_target,
    resolve_test_suite,
)


def test_registry_dicts_non_empty():
    assert PRESETS
    assert BUILD_TARGETS
    assert RUN_TARGETS
    assert GTEST_SUITES
    assert PYTEST_SUITES
    assert HEADER_ACTIONS
    assert TEST_SUITES


def test_test_suites_matches_gtest_and_pytest():
    expected = {**GTEST_SUITES, **{alias: None for alias in PYTEST_SUITES}}
    assert TEST_SUITES == expected


def test_pytest_suite_paths_exist():
    root = Path(__file__).resolve().parents[3]
    for alias, rel_path in PYTEST_SUITES.items():
        assert (root / rel_path).is_dir(), f"missing pytest dir for {alias}: {rel_path}"


def test_resolvers_return_expected_values():
    assert get_preset("x64-debug")["configure_preset"] == "x64-debug"
    assert resolve_build_target("editor") == "DeltaEditorLaunch"
    assert resolve_run_target("editor") == "DeltaEditorLaunch.exe"
    assert resolve_test_suite("engine") == "DeltaEngineTests.exe"
    assert resolve_test_suite("header-tool") is None
    assert resolve_test_suite("delta-cmd") is None
    assert resolve_pytest_suite("delta-cmd") == PYTEST_SUITES["delta-cmd"]


def test_resolvers_reject_unknown_aliases():
    with pytest.raises(RegistryError):
        get_preset("unknown")
    with pytest.raises(RegistryError):
        resolve_build_target("unknown")
    with pytest.raises(RegistryError):
        resolve_run_target("unknown")
    with pytest.raises(RegistryError):
        resolve_test_suite("unknown")
    with pytest.raises(RegistryError):
        resolve_pytest_suite("unknown")


def test_executable_path_shape():
    path = executable_path("x64-debug", "DeltaEngineTests.exe")
    assert path == Path("Build/x64-Debug/bin/DeltaEngineTests.exe")


def test_format_list_output_contains_all_aliases():
    output = format_list_output()

    for name in PRESETS:
        assert name in output
    for alias in BUILD_TARGETS:
        assert alias in output
        assert BUILD_TARGETS[alias] in output
    for alias in RUN_TARGETS:
        assert alias in output
        assert RUN_TARGETS[alias] in output
    for alias in GTEST_SUITES:
        assert alias in output
        assert GTEST_SUITES[alias] in output
    for alias in PYTEST_SUITES:
        assert alias in output
        assert str(PYTEST_SUITES[alias]) in output
    for action in HEADER_ACTIONS:
        assert action in output

    assert "Examples:" in output
    assert "DeltaCmd.bat configure --automatic" in output
    assert "DeltaCmd.bat list --automatic" in output
