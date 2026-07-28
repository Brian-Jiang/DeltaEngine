from __future__ import annotations

import json
import re
from pathlib import Path

REGISTER_QUERY_RE = re.compile(r'RegisterQuery\(\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,')
REGISTER_COMMAND_RE = re.compile(r'RegisterCommand\(\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,')

PYTHON_LOCAL_ONLY = frozenset({("meta", "query", "active_systems")})


def find_repo_root(start: Path | None = None) -> Path:
    here = (start or Path(__file__)).resolve()
    for parent in [here, *here.parents]:
        if (parent / "CMakePresets.json").exists():
            return parent
    raise RuntimeError("Could not locate repo root (CMakePresets.json not found)")


def parse_cpp_registrations(systems_dir: Path) -> dict[str, dict[str, set[str]]]:
    registry: dict[str, dict[str, set[str]]] = {}

    for cpp_path in sorted(systems_dir.glob("Mcp*System.cpp")):
        content = cpp_path.read_text(encoding="utf-8")

        for match in REGISTER_QUERY_RE.finditer(content):
            system, name = match.groups()
            registry.setdefault(system, {"queries": set(), "commands": set()})
            registry[system]["queries"].add(name)

        for match in REGISTER_COMMAND_RE.finditer(content):
            system, name = match.groups()
            registry.setdefault(system, {"queries": set(), "commands": set()})
            registry[system]["commands"].add(name)

    return registry


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
