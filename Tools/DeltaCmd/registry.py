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

GTEST_SUITES: dict[str, str] = {
    "engine": "DeltaEngineTests.exe",
    "editor": "DeltaEditorTests.exe",
}

PYTEST_SUITES: dict[str, Path] = {
    "header-tool": Path("Tools/DeltaHeaderTool/tests"),
    "delta-cmd": Path("Tools/DeltaCmd/tests"),
}

TEST_SUITES: dict[str, str | None] = {
    **GTEST_SUITES,
    **{alias: None for alias in PYTEST_SUITES},
}

HEADER_ACTIONS: dict[str, str] = {
    "generate": "Incremental reflection codegen",
    "force": "Full reflection codegen (--force)",
}

FORWARDER = "Tools/Scripts/DeltaCmd.bat"

MISSING_EXE_HINTS: dict[str, str] = {
    "editor": "Run 'DeltaCmd.bat build editor' or 'DeltaCmd.bat configure && DeltaCmd.bat build editor' first.",
    "engine": "Run 'DeltaCmd.bat build engine-tests' first.",
    "editor-tests": "Run 'DeltaCmd.bat build editor-tests' first.",
}

HEADER_TOOL_SCRIPT = Path("Tools/DeltaHeaderTool/main.py")
HEADER_TOOL_TEST_DIR = PYTEST_SUITES["header-tool"]

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


def resolve_pytest_suite(alias: str) -> Path:
    test_dir = PYTEST_SUITES.get(alias)
    if test_dir is None:
        known = ", ".join(sorted(PYTEST_SUITES))
        raise RegistryError(f"Unknown pytest suite '{alias}'. Known suites: {known}")
    return test_dir


def format_list_output() -> str:
    lines: list[str] = []
    lines.append("DeltaCmd registry")
    lines.append("=" * 17)
    lines.append("")

    lines.append("Presets:")
    for name in sorted(PRESETS):
        preset = PRESETS[name]
        configure_preset = preset["configure_preset"]
        binary_dir = preset["binary_dir"]
        lines.append(f"  {name}  (configure: {configure_preset}, binary: {binary_dir})")
    lines.append("")

    lines.append("Build targets:")
    for alias in sorted(BUILD_TARGETS):
        lines.append(f"  {alias:<14} -> {BUILD_TARGETS[alias]}")
    lines.append("")

    lines.append("Run targets:")
    for alias in sorted(RUN_TARGETS):
        lines.append(f"  {alias:<14} -> {RUN_TARGETS[alias]}")
    lines.append("")

    lines.append("Test suites:")
    for alias in sorted(GTEST_SUITES):
        lines.append(f"  {alias:<14} -> {GTEST_SUITES[alias]} (GTest)")
    for alias in sorted(PYTEST_SUITES):
        lines.append(f"  {alias:<14} -> pytest {PYTEST_SUITES[alias]}")
    lines.append("")

    lines.append("Header commands:")
    for action in sorted(HEADER_ACTIONS):
        lines.append(f"  {action:<9} {HEADER_ACTIONS[action]}")
    lines.append("")

    lines.append("Examples:")
    forwarder = FORWARDER.replace("/", "\\")
    lines.append(f"  {forwarder} configure --automatic")
    lines.append(f"  {forwarder} build editor --automatic")
    lines.append(f"  {forwarder} run editor --automatic")
    lines.append(f"  {forwarder} test engine --automatic")
    lines.append(f"  {forwarder} test delta-cmd --automatic")
    lines.append(f"  {forwarder} header generate --automatic")
    lines.append(f"  {forwarder} list --automatic")

    return "\n".join(lines)
