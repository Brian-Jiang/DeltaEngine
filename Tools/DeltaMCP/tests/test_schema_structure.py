from pathlib import Path

from schema_validation import (
    ALL_PARAM_TYPES,
    LEGACY_PARAM_TYPES,
    OPERATION_KEYS,
    PARAM_KEYS,
    SYSTEM_KEYS,
    TARGET_PARAM_TYPES,
    validate_all_schemas,
)

SCHEMAS_DIR = Path(__file__).resolve().parent.parent / "Schemas"

EXPECTED_SYSTEM_COUNTS = {
    "assets": (8, 2),
    "common": (0, 3),
    "lights": (0, 1),
    "log": (2, 0),
    "meta": (3, 0),
    "project": (4, 1),
    "reflection": (4, 0),
    "scene": (6, 9),
    "selection": (1, 1),
    "undo_history": (1, 2),
    "viewport": (3, 1),
}

EXPECTED_QUERY_NAMES = {
    "assets": [
        "folder_tree",
        "get",
        "get_asset_metadata",
        "get_assets_metadata",
        "has_static_meta_schema",
        "list",
        "search",
        "usages",
    ],
    "common": [],
    "lights": [],
    "log": ["file_location", "read"],
    "meta": ["capabilities", "describe_operations", "list_operations"],
    "project": ["build_state", "info", "open_scenes", "settings"],
    "reflection": [
        "class_schema",
        "classes",
        "find_classes_with_property",
        "inheritance_chain",
    ],
    "scene": [
        "component",
        "components_on_object",
        "find_by_property",
        "game_object",
        "game_objects",
        "hierarchy",
    ],
    "selection": ["current"],
    "undo_history": ["stack"],
    "viewport": ["camera", "render_settings", "visible_objects"],
}

EXPECTED_COMMAND_NAMES = {
    "assets": ["reimport_assets", "set_asset_dynamic_metadata"],
    "common": ["RenameObject", "SaveProject", "SetProperty"],
    "lights": ["SetIntensity"],
    "log": [],
    "meta": [],
    "project": ["LoadScene"],
    "reflection": [],
    "scene": [
        "CreateComponent",
        "CreateGameObject",
        "DeleteComponent",
        "DeleteGameObject",
        "DuplicateGameObject",
        "ReparentSceneComponent",
        "SetPosition",
        "SetRotation",
        "SetScale",
    ],
    "selection": ["SelectObject"],
    "undo_history": ["Redo", "Undo"],
    "viewport": ["SetViewportCamera"],
}

# Baseline permissive-mode violation counts (Phase 1 pre-migration).
EXPECTED_VIOLATION_COUNTS = {
    "legacy_optional": 46,
    "legacy_param_type": 4,
    "missing_commands_key": 0,
    "missing_param_required": 4,
    "missing_param_type": 2,
    "required_false_without_default": 7,
    "unknown_param_key": 46,
}


def test_allowed_key_constants_documented():
    assert SYSTEM_KEYS == frozenset({"system", "description", "queries", "commands"})
    assert OPERATION_KEYS == frozenset(
        {
            "description",
            "params",
            "undoable",
            "expects_result",
            "expects_result_note",
            "returns",
        }
    )
    assert PARAM_KEYS == frozenset(
        {"type", "required", "description", "default", "options", "items"}
    )
    assert TARGET_PARAM_TYPES == frozenset(
        {"string", "bool", "int", "float", "array", "object", "any"}
    )
    assert LEGACY_PARAM_TYPES == frozenset({"integer", "number"})
    assert ALL_PARAM_TYPES == TARGET_PARAM_TYPES | LEGACY_PARAM_TYPES


def test_all_schema_files_load_permissive():
    report = validate_all_schemas(SCHEMAS_DIR, strict=False)
    assert report.ok is True
    assert len(report.files) == 11
    assert report.system_count == 11
    assert report.total_queries == 32
    assert report.total_commands == 20


def test_per_system_operation_counts():
    report = validate_all_schemas(SCHEMAS_DIR, strict=False)
    assert set(report.systems.keys()) == set(EXPECTED_SYSTEM_COUNTS.keys())

    for system, (query_count, command_count) in EXPECTED_SYSTEM_COUNTS.items():
        summary = report.systems[system]
        assert len(summary.queries) == query_count, system
        assert len(summary.commands) == command_count, system
        assert summary.queries == EXPECTED_QUERY_NAMES[system], system
        assert summary.commands == EXPECTED_COMMAND_NAMES[system], system


def test_permissive_violation_baseline():
    report = validate_all_schemas(SCHEMAS_DIR, strict=False)
    counts = report.violation_counts()

    assert counts["legacy_optional"] >= 1
    assert counts["missing_param_type"] >= 1
    assert counts["unknown_param_key"] >= 1

    optional_unknown = [
        v
        for v in report.violations
        if v.code == "unknown_param_key" and v.path.endswith("/optional")
    ]
    assert len(optional_unknown) == counts["legacy_optional"]

    for code, expected in EXPECTED_VIOLATION_COUNTS.items():
        assert counts[code] == expected, f"{code}: got {counts[code]}, expected {expected}"


def test_strict_mode_would_fail():
    # Documents pre-migration state; strict cleanliness is a later phase.
    report = validate_all_schemas(SCHEMAS_DIR, strict=True)
    assert report.ok is False
    assert len(report.errors) > 0


def _format_violation_snapshot(report) -> str:
    lines = []
    for v in sorted(report.violations, key=lambda x: (x.path, x.code)):
        lines.append(f"{v.path}\t{v.code}")
    return "\n".join(lines) + "\n"


def test_permissive_violation_snapshot(snapshot_dir, snapshot_update):
    report = validate_all_schemas(SCHEMAS_DIR, strict=False)
    snapshot_path = snapshot_dir / "violations_permissive.txt"
    content = _format_violation_snapshot(report)

    if snapshot_update:
        snapshot_dir.mkdir(parents=True, exist_ok=True)
        snapshot_path.write_text(content, encoding="utf-8")
        return

    assert snapshot_path.is_file(), "missing snapshot; run pytest with --snapshot-update"
    assert content == snapshot_path.read_text(encoding="utf-8")
