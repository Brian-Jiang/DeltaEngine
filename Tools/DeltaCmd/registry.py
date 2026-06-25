from __future__ import annotations

from pathlib import Path

PRESETS: dict[str, dict[str, str | Path]] = {
    "x64-debug": {
        "configure_preset": "x64-debug",
        "binary_dir": Path("Build/x64-Debug"),
    },
}

BUILD_TARGETS: dict[str, str] = {
    "editor": "DeltaEditorLaunch",
    "engine-tests": "DeltaEngineTests",
    "editor-tests": "DeltaEditorTests",
}

RUN_TARGETS: dict[str, str] = {
    "editor": "DeltaEditorLaunch.exe",
}

TEST_SUITES: dict[str, str | None] = {
    "engine": "DeltaEngineTests.exe",
    "editor": "DeltaEditorTests.exe",
    "header-tool": None,
}

MISSING_EXE_HINTS: dict[str, str] = {
    "editor": "Run build-x64-debug.bat or rebuild-x64-debug.bat first.",
    "engine": "Run build-x64-debug-engine-tests.bat first.",
    "editor-tests": "Run build-x64-debug-editor-tests.bat first.",
}

HEADER_TOOL_SCRIPT = Path("Tools/DeltaHeaderTool/main.py")
HEADER_TOOL_TEST_DIR = Path("Tools/DeltaHeaderTool/tests")

HEADER_TOOL_INCLUDE_DIRS: tuple[str, ...] = (
    "Engine",
    "Engine/Runtime",
    "Engine/ThirdParty/DirectXTK12/Inc",
    "Engine/ThirdParty/DirectX-Headers/include/directx",
)


class RegistryError(Exception):
    pass


def get_preset(name: str) -> dict[str, str | Path]:
    preset = PRESETS.get(name)
    if preset is None:
        known = ", ".join(sorted(PRESETS))
        raise RegistryError(f"Unknown preset '{name}'. Known presets: {known}")
    return preset


def resolve_build_target(alias: str) -> str:
    target = BUILD_TARGETS.get(alias)
    if target is None:
        known = ", ".join(sorted(BUILD_TARGETS))
        raise RegistryError(f"Unknown build target '{alias}'. Known targets: {known}")
    return target


def resolve_run_target(alias: str) -> str:
    target = RUN_TARGETS.get(alias)
    if target is None:
        known = ", ".join(sorted(RUN_TARGETS))
        raise RegistryError(f"Unknown run target '{alias}'. Known targets: {known}")
    return target


def resolve_test_suite(alias: str) -> str | None:
    if alias not in TEST_SUITES:
        known = ", ".join(sorted(TEST_SUITES))
        raise RegistryError(f"Unknown test suite '{alias}'. Known suites: {known}")
    return TEST_SUITES[alias]


def executable_path(preset_name: str, exe_name: str) -> Path:
    preset = get_preset(preset_name)
    binary_dir = preset["binary_dir"]
    if not isinstance(binary_dir, Path):
        raise RegistryError("Invalid preset binary_dir.")
    return binary_dir / "bin" / exe_name


def missing_run_hint(alias: str) -> str:
    return MISSING_EXE_HINTS.get(alias, "Build the target first.")


def missing_test_hint(alias: str) -> str:
    if alias == "engine":
        return MISSING_EXE_HINTS["engine"]
    if alias == "editor":
        return MISSING_EXE_HINTS["editor-tests"]
    return "Build the target first."


def header_tool_argv(project_root: Path, *, force: bool) -> list[str]:
    generated_dir = project_root / "Intermediate" / "DeltaHeaderTool" / "Generated"
    manifest = project_root / "Intermediate" / "DeltaHeaderTool" / "generated_sources.cmake"

    argv = [
        "--input-dir",
        "Engine/Runtime",
        "--output-dir",
        str(generated_dir),
        "--manifest",
        str(manifest),
        "--engine-root",
        str(project_root),
    ]

    for include_dir in HEADER_TOOL_INCLUDE_DIRS:
        argv.extend(["--include-dir", str(project_root / include_dir)])

    if force:
        argv.append("--force")

    return argv
