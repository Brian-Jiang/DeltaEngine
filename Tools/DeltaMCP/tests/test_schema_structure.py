from pathlib import Path

import json

from schema_validation import (
    ALL_PARAM_TYPES,
    LEGACY_PARAM_TYPES,
    NO_DEFAULT_EXCEPTIONS,
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
    "meta": (4, 0),
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
    "meta": ["active_systems", "capabilities", "describe_operations", "list_operations"],
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

# Baseline permissive-mode violation counts (Phase 4 post default migration).
EXPECTED_VIOLATION_COUNTS = {
    "legacy_optional": 0,
    "legacy_param_type": 0,
    "missing_commands_key": 0,
    "missing_param_required": 0,
    "missing_param_type": 0,
    "required_false_without_default": 0,
    "unknown_param_key": 0,
}


def _iter_all_param_paths(schemas_dir: Path):
    """Yield (path, param_spec) for every param in every schema file."""
    for path in sorted(schemas_dir.glob("*.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        for kind in ("queries", "commands"):
            for op_name, op_spec in data.get(kind, {}).items():
                for param_name, param_spec in op_spec.get("params", {}).items():
                    yield (
                        f"{path.name}/{kind}/{op_name}/params/{param_name}",
                        param_spec,
                    )


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
    assert report.total_queries == 33
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

    assert counts["legacy_optional"] == 0
    assert counts.get("missing_param_type", 0) == 0
    assert counts.get("legacy_param_type", 0) == 0
    assert counts["unknown_param_key"] == 0

    for code, expected in EXPECTED_VIOLATION_COUNTS.items():
        assert counts[code] == expected, f"{code}: got {counts[code]}, expected {expected}"


def test_every_param_has_canonical_type():
    report = validate_all_schemas(SCHEMAS_DIR, strict=False)
    counts = report.violation_counts()
    assert counts.get("missing_param_type", 0) == 0
    assert counts.get("legacy_param_type", 0) == 0


def test_strict_mode_passes():
    report = validate_all_schemas(SCHEMAS_DIR, strict=True)
    assert report.ok is True, report.violations
    assert report.violation_counts().get("required_false_without_default", 0) == 0
    assert report.violation_counts().get("required_true_with_default", 0) == 0


def test_no_default_exceptions_are_documented():
    all_paths = {path for path, _ in _iter_all_param_paths(SCHEMAS_DIR)}
    for exc_path in NO_DEFAULT_EXCEPTIONS:
        assert exc_path in all_paths, f"exception path not in schemas: {exc_path}"


def test_required_true_params_have_no_default():
    for path, spec in _iter_all_param_paths(SCHEMAS_DIR):
        if spec.get("required") is True:
            assert "default" not in spec, f"{path} has required:true with default"


def test_default_values_match_declared_type():
    report = validate_all_schemas(SCHEMAS_DIR, strict=True)
    mismatches = [v for v in report.violations if v.code == "default_type_mismatch"]
    assert mismatches == []


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
