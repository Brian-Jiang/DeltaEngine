from pathlib import Path

import delta_mcp_server
from registry_sync import (
    PYTHON_LOCAL_ONLY,
    compare_operation_registries,
    find_repo_root,
    load_schema_registry,
    parse_cpp_registrations,
)

REPO_ROOT = find_repo_root(Path(__file__))
CPP_SYSTEMS_DIR = REPO_ROOT / "Engine/Editor/Mcp/Systems"
SCHEMAS_DIR = REPO_ROOT / "Tools/DeltaMCP/Schemas"


def test_cpp_registry_matches_schema_operations():
    cpp_registry = parse_cpp_registrations(CPP_SYSTEMS_DIR)
    schema_registry = load_schema_registry(SCHEMAS_DIR)
    errors = compare_operation_registries(cpp_registry, schema_registry)
    assert errors == [], "\n".join(errors)


def test_python_local_only_entries_exist_in_schema():
    schema_registry = load_schema_registry(SCHEMAS_DIR)
    missing = []
    for system, kind, name in sorted(PYTHON_LOCAL_ONLY):
        bucket = "queries" if kind == "query" else "commands"
        if name not in schema_registry.get(system, {}).get(bucket, set()):
            missing.append(f"{system}/{name}")
    assert missing == [], (
        "Python-local operations must be documented in schema: "
        + ", ".join(missing)
    )


def test_local_meta_queries_subset_of_schema_meta_queries():
    meta_queries = load_schema_registry(SCHEMAS_DIR)["meta"]["queries"]
    for query in delta_mcp_server._LOCAL_META_QUERIES:
        assert query in meta_queries, (
            f"_LOCAL_META_QUERIES entry {query!r} missing from meta.json"
        )
