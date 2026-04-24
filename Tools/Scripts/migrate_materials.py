#!/usr/bin/env python3
"""Adds missing PBR fields to existing DMaterial .dasset.json files in-place."""

import json
import os
import sys
from pathlib import Path

DEFAULTS = {
    "m_baseColor": [1.0, 1.0, 1.0, 1.0],
    "m_metallic": 0.0,
    "m_roughness": 0.5,
    "m_emissiveColor": [0.0, 0.0, 0.0, 0.0],
    "m_emissiveIntensity": 1.0,
    "m_alphaCutoff": 0.5,
    "m_flags": 0,
}


def migrate_object(obj: dict) -> bool:
    changed = False
    for key, default_value in DEFAULTS.items():
        if key not in obj:
            obj[key] = default_value
            changed = True
    return changed


def migrate_file(path: Path) -> bool:
    try:
        with path.open("r", encoding="utf-8") as f:
            data = json.load(f)
    except (json.JSONDecodeError, OSError):
        return False

    if not isinstance(data, dict):
        return False

    objects = data.get("objects")
    if not isinstance(objects, list):
        return False

    changed = False
    for obj in objects:
        if not isinstance(obj, dict):
            continue
        if obj.get("_class") != "DMaterial":
            continue
        if migrate_object(obj):
            changed = True

    if changed:
        with path.open("w", encoding="utf-8", newline="\n") as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
            f.write("\n")
    return changed


def main() -> int:
    project_root = Path(__file__).resolve().parent.parent.parent
    engine_root = project_root / "Engine"
    if not engine_root.is_dir():
        print(f"ERROR: Engine directory not found at {engine_root}", file=sys.stderr)
        return 1

    total = 0
    modified = 0
    for path in engine_root.rglob("*.dasset.json"):
        total += 1
        if migrate_file(path):
            modified += 1
            print(f"  migrated: {path.relative_to(project_root)}")

    print(f"\nScanned {total} .dasset.json files, migrated {modified}.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
