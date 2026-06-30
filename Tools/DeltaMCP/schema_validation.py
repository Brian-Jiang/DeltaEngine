from __future__ import annotations

import argparse
import json
import sys
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path


SYSTEM_KEYS = frozenset({"system", "description", "queries", "commands"})
OPERATION_KEYS = frozenset(
    {"description", "params", "undoable", "expects_result", "expects_result_note", "returns"}
)
PARAM_KEYS = frozenset({"type", "required", "description", "default", "options", "items"})
# Canonical param types: int for integers, float for floating-point scalars.
# LEGACY_PARAM_TYPES (integer, number) are rejected in strict mode via legacy_param_type.
TARGET_PARAM_TYPES = frozenset({"string", "bool", "int", "float", "array", "object", "any"})
LEGACY_PARAM_TYPES = frozenset({"integer", "number"})
ALL_PARAM_TYPES = TARGET_PARAM_TYPES | LEGACY_PARAM_TYPES

# required: false params that correctly omit "default" — no static C++ default exists.
# Runtime-resolved, mode-switching, merge-into-current, or mutually-exclusive params.
NO_DEFAULT_EXCEPTIONS = frozenset({
    "assets.json/queries/get_assets_metadata/params/asset_ids",
    "assets.json/queries/has_static_meta_schema/params/asset_id",
    "assets.json/queries/has_static_meta_schema/params/class",
    "common.json/commands/RenameObject/params/assetId",
    "common.json/commands/SetProperty/params/assetId",
    "log.json/queries/read/params/since_seconds",
    "scene.json/commands/DuplicateGameObject/params/offset_position",
    "scene.json/queries/hierarchy/params/root_object_id",
    "viewport.json/commands/SetViewportCamera/params/fov",
    "viewport.json/commands/SetViewportCamera/params/position",
    "viewport.json/commands/SetViewportCamera/params/rotation",
})


def _is_int(v):
    return isinstance(v, int) and not isinstance(v, bool)


def _is_number(v):
    return isinstance(v, (int, float)) and not isinstance(v, bool)


_DEFAULT_TYPE_VALIDATORS = {
    "string": lambda v: isinstance(v, str),
    "bool": lambda v: isinstance(v, bool),
    "int": _is_int,
    "integer": _is_int,
    "float": _is_number,
    "number": _is_number,
    "array": lambda v: isinstance(v, list),
    "object": lambda v: isinstance(v, dict),
    "any": lambda v: True,
}


@dataclass(frozen=True)
class Violation:
    path: str
    code: str
    message: str
    severity: str


@dataclass
class SystemSummary:
    queries: list[str] = field(default_factory=list)
    commands: list[str] = field(default_factory=list)


@dataclass
class SchemaValidationReport:
    files: list[str] = field(default_factory=list)
    systems: dict[str, SystemSummary] = field(default_factory=dict)
    total_queries: int = 0
    total_commands: int = 0
    system_count: int = 0
    violations: list[Violation] = field(default_factory=list)

    @property
    def ok(self) -> bool:
        return not any(v.severity == "error" for v in self.violations)

    @property
    def errors(self) -> list[Violation]:
        return [v for v in self.violations if v.severity == "error"]

    @property
    def warnings(self) -> list[Violation]:
        return [v for v in self.violations if v.severity == "warning"]

    def violation_counts(self) -> Counter[str]:
        return Counter(v.code for v in self.violations)


def _add(violations: list[Violation], path: str, code: str, message: str, *, strict: bool) -> None:
    severity = "error" if strict else "warning"
    violations.append(Violation(path=path, code=code, message=message, severity=severity))


def _validate_default_value(
    violations: list[Violation],
    path: str,
    type_name: str | None,
    default,
    *,
    strict: bool,
) -> None:
    if type_name is None:
        return
    validator = _DEFAULT_TYPE_VALIDATORS.get(type_name)
    if validator is None:
        return
    if not validator(default):
        _add(
            violations,
            path,
            "default_type_mismatch",
            f"default value {default!r} is incompatible with type '{type_name}'",
            strict=strict,
        )


def _validate_param(
    violations: list[Violation],
    path: str,
    param: dict,
    *,
    strict: bool,
) -> None:
    if not isinstance(param, dict):
        _add(violations, path, "invalid_param", "Param must be a JSON object", strict=True)
        return

    for key in param:
        if key not in PARAM_KEYS:
            _add(
                violations,
                f"{path}/{key}",
                "unknown_param_key",
                f"Unknown param key '{key}'",
                strict=strict,
            )

    if "optional" in param:
        _add(
            violations,
            path,
            "legacy_optional",
            "Use 'required: false' instead of 'optional: true'",
            strict=strict,
        )

    has_required = "required" in param
    has_optional = "optional" in param
    if not has_required and not has_optional:
        _add(
            violations,
            path,
            "missing_param_required",
            "Param must specify 'required' (true or false)",
            strict=strict,
        )

    type_name = param.get("type")
    if type_name is None:
        _add(
            violations,
            path,
            "missing_param_type",
            "Param must specify 'type'",
            strict=True,
        )
    elif type_name in LEGACY_PARAM_TYPES:
        _add(
            violations,
            path,
            "legacy_param_type",
            f"Legacy type '{type_name}' (use int or float)",
            strict=strict,
        )
    elif type_name not in TARGET_PARAM_TYPES:
        _add(
            violations,
            path,
            "invalid_param_type",
            f"Invalid type '{type_name}'",
            strict=strict,
        )

    required_val = param.get("required")
    if required_val is True and "default" in param:
        _add(
            violations,
            path,
            "required_true_with_default",
            "required: true must not have a default",
            strict=strict,
        )

    if required_val is False and "default" not in param and path not in NO_DEFAULT_EXCEPTIONS:
        _add(
            violations,
            path,
            "required_false_without_default",
            "required: false should include a default when C++ applies one",
            strict=strict,
        )

    options = param.get("options")
    if options is not None:
        if not isinstance(options, list) or len(options) == 0:
            _add(
                violations,
                path,
                "invalid_options",
                "'options' must be a non-empty list",
                strict=True,
            )

    items = param.get("items")
    if items is not None:
        if not isinstance(items, dict) or "type" not in items:
            _add(
                violations,
                path,
                "invalid_items",
                "'items' must be an object with a 'type' key",
                strict=True,
            )
        else:
            item_type = items["type"]
            if item_type not in TARGET_PARAM_TYPES:
                _add(
                    violations,
                    f"{path}/items",
                    "invalid_items_type",
                    f"Invalid items type '{item_type}'",
                    strict=strict,
                )

    if "default" in param:
        _validate_default_value(
            violations,
            path,
            type_name,
            param["default"],
            strict=strict,
        )


def _validate_operation(
    violations: list[Violation],
    path: str,
    operation: dict,
    *,
    strict: bool,
) -> None:
    if not isinstance(operation, dict):
        _add(violations, path, "invalid_operation", "Operation must be a JSON object", strict=True)
        return

    for key in operation:
        if key not in OPERATION_KEYS:
            _add(
                violations,
                f"{path}/{key}",
                "unknown_operation_key",
                f"Unknown operation key '{key}'",
                strict=strict,
            )

    description = operation.get("description")
    if not isinstance(description, str):
        _add(
            violations,
            path,
            "missing_operation_description",
            "Operation must have a string 'description'",
            strict=True,
        )

    params = operation.get("params")
    if params is None:
        _add(
            violations,
            path,
            "missing_operation_params",
            "Operation must have a 'params' object",
            strict=True,
        )
        return

    if not isinstance(params, dict):
        _add(
            violations,
            path,
            "invalid_operation_params",
            "'params' must be a JSON object",
            strict=True,
        )
        return

    for param_name, param_spec in params.items():
        _validate_param(violations, f"{path}/params/{param_name}", param_spec, strict=strict)


def validate_schema_file(path: Path, data: dict, *, strict: bool = False) -> list[Violation]:
    violations: list[Violation] = []
    file_label = path.name

    if not isinstance(data, dict):
        _add(violations, file_label, "invalid_root", "Schema root must be a JSON object", strict=True)
        return violations

    for key in data:
        if key not in SYSTEM_KEYS:
            _add(
                violations,
                f"{file_label}/{key}",
                "unknown_system_key",
                f"Unknown system key '{key}'",
                strict=strict,
            )

    system_name = data.get("system")
    if not isinstance(system_name, str) or not system_name:
        _add(
            violations,
            file_label,
            "missing_system_name",
            "System must have a non-empty string 'system'",
            strict=True,
        )

    if not isinstance(data.get("description"), str):
        _add(
            violations,
            file_label,
            "missing_system_description",
            "System must have a string 'description'",
            strict=True,
        )

    if "commands" not in data:
        _add(
            violations,
            file_label,
            "missing_commands_key",
            "System should include an explicit 'commands' key",
            strict=strict,
        )

    for kind in ("queries", "commands"):
        block = data.get(kind)
        if block is None:
            continue
        if not isinstance(block, dict):
            _add(
                violations,
                f"{file_label}/{kind}",
                f"invalid_{kind}",
                f"'{kind}' must be a JSON object",
                strict=True,
            )
            continue
        for op_name, op_spec in block.items():
            _validate_operation(
                violations,
                f"{file_label}/{kind}/{op_name}",
                op_spec,
                strict=strict,
            )

    return violations


def validate_all_schemas(schemas_dir: Path, *, strict: bool = False) -> SchemaValidationReport:
    report = SchemaValidationReport()

    for path in sorted(schemas_dir.glob("*.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        report.files.append(path.name)

        file_violations = validate_schema_file(path, data, strict=strict)
        report.violations.extend(file_violations)

        if not isinstance(data, dict) or "system" not in data:
            continue

        system_name = data["system"]
        summary = SystemSummary(
            queries=sorted(data.get("queries", {}).keys()),
            commands=sorted(data.get("commands", {}).keys()),
        )
        report.systems[system_name] = summary
        report.total_queries += len(summary.queries)
        report.total_commands += len(summary.commands)

    report.system_count = len(report.systems)
    return report


def _print_report(report: SchemaValidationReport) -> None:
    print(f"Files: {len(report.files)}")
    print(f"Systems: {report.system_count}")
    print(f"Queries: {report.total_queries}")
    print(f"Commands: {report.total_commands}")
    print(f"Violations: {len(report.violations)} "
          f"({len(report.errors)} errors, {len(report.warnings)} warnings)")

    counts = report.violation_counts()
    if counts:
        print("\nBy code:")
        for code, count in sorted(counts.items()):
            print(f"  {code}: {count}")

    if report.violations:
        print("\nDetails:")
        for v in sorted(report.violations, key=lambda x: (x.severity, x.path, x.code)):
            print(f"  [{v.severity}] {v.path}: {v.code} — {v.message}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Validate DeltaMCP schema JSON structure")
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Treat legacy/unknown keys as errors",
    )
    parser.add_argument(
        "--schemas-dir",
        type=Path,
        default=None,
        help="Path to Schemas directory (default: sibling Schemas/ of this script)",
    )
    args = parser.parse_args(argv)

    schemas_dir = args.schemas_dir or (Path(__file__).resolve().parent / "Schemas")
    report = validate_all_schemas(schemas_dir, strict=args.strict)
    _print_report(report)
    return 0 if report.ok else 1


if __name__ == "__main__":
    sys.exit(main())
