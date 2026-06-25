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
