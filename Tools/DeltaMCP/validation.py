import difflib

from schema_validation import TARGET_PARAM_TYPES


def _is_int(v):
    return isinstance(v, int) and not isinstance(v, bool)


def _is_number(v):
    return isinstance(v, (int, float)) and not isinstance(v, bool)


_TYPE_VALIDATORS = {
    "string": lambda v: isinstance(v, str),
    "bool": lambda v: isinstance(v, bool),
    "int": _is_int,
    "float": _is_number,
    "array": lambda v: isinstance(v, list),
    "object": lambda v: isinstance(v, dict),
    "any": lambda v: True,
}


def _validate_params(params, param_schemas):
    errors = []
    for name, spec in param_schemas.items():
        if spec.get("required") is True and name not in params:
            errors.append({
                "param": name,
                "problem": "missing_required",
                "expected": spec.get("type", "any"),
            })

    for name, value in params.items():
        spec = param_schemas.get(name)
        if spec is None:
            errors.append({
                "param": name,
                "problem": "unknown",
            })
            continue

        type_name = spec.get("type")
        if type_name is not None:
            if type_name not in TARGET_PARAM_TYPES:
                errors.append({
                    "param": name,
                    "problem": "unknown_type",
                    "expected": sorted(TARGET_PARAM_TYPES),
                    "got": type_name,
                })
                continue

            validator = _TYPE_VALIDATORS.get(type_name)
            if validator is not None and not validator(value):
                errors.append({
                    "param": name,
                    "problem": "type_mismatch",
                    "expected": type_name,
                    "got": type(type).__name__,
                })
                continue

        options = spec.get("options")
        if options is not None and value not in options:
            errors.append({
                "param": name,
                "problem": "invalid_option",
                "expected": options,
                "got": value,
            })

    return errors


def validate_operation(op, systems):
    if not isinstance(op, dict):
        return {
            "ok": False,
            "stage": "envelope",
            "error": "Operation must be a JSON object",
        }

    op_type = op.get("type", "query")
    if op_type not in ("query", "command"):
        return {
            "ok": False,
            "stage": "envelope",
            "error": f"Invalid operation type '{op_type}' (expected 'query' or 'command')",
        }

    kind = "commands" if op_type == "command" else "queries"
    name_field = "command" if op_type == "command" else "query"

    system = op.get("system", "")
    name = op.get(name_field, "")

    if not system or not name:
        return {
            "ok": False,
            "stage": "envelope",
            "error": f"Operation must include non-empty 'system' and '{name_field}' fields",
        }

    sys_schema = systems.get(system)
    if sys_schema is None:
        return {
            "ok": False,
            "stage": "system",
            "error": f"Unknown system '{system}'",
            "available_systems": sorted(systems.keys()),
        }

    operations = sys_schema.get(kind, {})
    op_schema = operations.get(name)
    if op_schema is None:
        available = sorted(operations.keys())
        return {
            "ok": False,
            "stage": "operation",
            "error": f"Unknown {op_type} '{name}' on system '{system}'",
            "available": available,
            "did_you_mean": difflib.get_close_matches(name, available, n=3),
        }

    params = op.get("params", {})
    if not isinstance(params, dict):
        return {
            "ok": False,
            "stage": "params",
            "error": f"'params' for {system}/{name} must be a JSON object",
            "schema": op_schema,
        }

    param_errors = _validate_params(params, op_schema.get("params", {}))
    if param_errors:
        return {
            "ok": False,
            "stage": "params",
            "error": f"Invalid params for {system}/{name}",
            "param_errors": param_errors,
            "schema": op_schema,
        }

    return None
