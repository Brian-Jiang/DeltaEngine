from __future__ import annotations

import json
import re
from dataclasses import dataclass
from pathlib import Path

REGISTER_QUERY_RE = re.compile(
    r'RegisterQuery\(\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*'
    r'\[this\]\([^)]*\)\s*\{\s*return\s+(\w+)\('
)
REGISTER_COMMAND_RE = re.compile(
    r'RegisterCommand\(\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*'
    r'\[this\]\([^)]*\)\s*\{\s*return\s+(\w+)\('
)

PARAM_ACCESS_RE = re.compile(
    r'params\.(?:contains|value)\(\s*"([^"]+)"|params\[\s*"([^"]+)"\s*\]'
)

HELPER_CALL_RE = re.compile(r'\b(\w+)\([^)]*\bparams\b[^)]*\)')

FUNCTION_START_RE = re.compile(
    r'(?:^|\n)(?:static\s+)?[\w:<>*&\s]+\b({name})\s*\([^)]*\)\s*\{{',
    re.MULTILINE,
)

PYTHON_LOCAL_ONLY = frozenset({("meta", "query", "active_systems")})


@dataclass(frozen=True)
class OperationRegistration:
    system: str
    kind: str
    name: str
    handler: str
    source_file: Path


def find_repo_root(start: Path | None = None) -> Path:
    here = (start or Path(__file__)).resolve()
    for parent in [here, *here.parents]:
        if (parent / "CMakePresets.json").exists():
            return parent
    raise RuntimeError("Could not locate repo root (CMakePresets.json not found)")


def _extract_balanced_body(content: str, open_brace_index: int) -> str:
    depth = 0
    for index in range(open_brace_index, len(content)):
        char = content[index]
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return content[open_brace_index + 1 : index]
    raise ValueError("Unbalanced braces while extracting function body")


def extract_function_body(content: str, func_name: str) -> str | None:
    pattern = FUNCTION_START_RE.pattern.format(name=re.escape(func_name))
    match = re.search(pattern, content, re.MULTILINE)
    if not match:
        return None
    open_brace = content.index("{", match.start())
    return _extract_balanced_body(content, open_brace)


def extract_params_from_body(body: str) -> set[str]:
    params: set[str] = set()
    for match in PARAM_ACCESS_RE.finditer(body):
        key = match.group(1) or match.group(2)
        if key:
            params.add(key)
    return params


def extract_handler_params(content: str, handler_name: str) -> set[str]:
    body = extract_function_body(content, handler_name)
    if body is None:
        return set()

    params = extract_params_from_body(body)
    for helper_match in HELPER_CALL_RE.finditer(body):
        helper_name = helper_match.group(1)
        if helper_name == handler_name:
            continue
        helper_body = extract_function_body(content, helper_name)
        if helper_body is not None:
            params |= extract_params_from_body(helper_body)
    return params


def parse_cpp_registrations(systems_dir: Path) -> tuple[
    dict[str, dict[str, set[str]]],
    list[OperationRegistration],
]:
    registry: dict[str, dict[str, set[str]]] = {}
    operations: list[OperationRegistration] = []

    for cpp_path in sorted(systems_dir.glob("Mcp*System.cpp")):
        content = cpp_path.read_text(encoding="utf-8")

        for match in REGISTER_QUERY_RE.finditer(content):
            system, name, handler = match.groups()
            registry.setdefault(system, {"queries": set(), "commands": set()})
            registry[system]["queries"].add(name)
            operations.append(
                OperationRegistration(system, "query", name, handler, cpp_path)
            )

        for match in REGISTER_COMMAND_RE.finditer(content):
            system, name, handler = match.groups()
            registry.setdefault(system, {"queries": set(), "commands": set()})
            registry[system]["commands"].add(name)
            operations.append(
                OperationRegistration(system, "command", name, handler, cpp_path)
            )

    return registry, operations


def load_schema_registry(schemas_dir: Path) -> dict[str, dict[str, set[str]]]:
    registry: dict[str, dict[str, set[str]]] = {}
    for path in sorted(schemas_dir.glob("*.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        system = data["system"]
        registry[system] = {
            "queries": set(data.get("queries", {})),
            "commands": set(data.get("commands", {})),
        }
    return registry


def load_schema_params(schemas_dir: Path) -> dict[tuple[str, str, str], set[str]]:
    params_by_operation: dict[tuple[str, str, str], set[str]] = {}
    for path in sorted(schemas_dir.glob("*.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        system = data["system"]
        for kind in ("queries", "commands"):
            op_kind = "query" if kind == "queries" else "command"
            for op_name, op_spec in data.get(kind, {}).items():
                params_by_operation[(system, op_kind, op_name)] = set(
                    op_spec.get("params", {}).keys()
                )
    return params_by_operation


def compare_operation_registries(
    cpp_registry: dict[str, dict[str, set[str]]],
    schema_registry: dict[str, dict[str, set[str]]],
) -> list[str]:
    errors: list[str] = []
    all_systems = set(cpp_registry) | set(schema_registry)

    for system in sorted(all_systems):
        cpp = cpp_registry.get(system, {"queries": set(), "commands": set()})
        schema = schema_registry.get(system, {"queries": set(), "commands": set()})

        for kind in ("queries", "commands"):
            op_kind = "query" if kind == "queries" else "command"
            cpp_names = cpp[kind]
            schema_names = schema[kind]

            for name in sorted(cpp_names - schema_names):
                errors.append(
                    f"C++ {op_kind} {system}/{name} has no schema entry"
                )

            for name in sorted(schema_names - cpp_names):
                key = (system, op_kind, name)
                if key in PYTHON_LOCAL_ONLY:
                    continue
                errors.append(
                    f"Schema {op_kind} {system}/{name} has no C++ Register{op_kind.title()} handler"
                )

    return errors


def compare_operation_params(
    operations: list[OperationRegistration],
    schema_params: dict[tuple[str, str, str], set[str]],
) -> list[str]:
    errors: list[str] = []
    file_cache: dict[Path, str] = {}

    for op in operations:
        if op.source_file not in file_cache:
            file_cache[op.source_file] = op.source_file.read_text(encoding="utf-8")
        content = file_cache[op.source_file]

        handler_params = extract_handler_params(content, op.handler)
        schema_op_params = schema_params.get((op.system, op.kind, op.name), set())

        for param in sorted(schema_op_params - handler_params):
            errors.append(
                f"Schema param {op.system}/{op.name}.{param} is not read by C++ handler {op.handler}"
            )

        for param in sorted(handler_params - schema_op_params):
            errors.append(
                f"C++ handler {op.system}/{op.name}.{param} ({op.handler}) is not documented in schema"
            )

    return errors
