from __future__ import annotations

import sys
from dataclasses import dataclass, field
from pathlib import Path

import clang.cindex as ci

from type_resolver import resolve_type

TOOL_DIR = Path(__file__).resolve().parent
TOOLS_DIR = TOOL_DIR.parent
LIB_PATH = TOOLS_DIR / "Clang"

_configured = False


def _ensure_configured():
    global _configured
    if not _configured:
        dll = LIB_PATH / "libclang.dll"
        if not dll.exists():
            print(
                f"ERROR: libclang.dll not found at {dll}\n"
                "Place the LLVM libclang.dll in Tools/Clang/.",
                file=sys.stderr,
            )
            sys.exit(1)
        ci.Config.set_library_path(str(LIB_PATH))
        _configured = True


# ── data model ────────────────────────────────────────────────


@dataclass
class PropertyInfo:
    name: str
    cpp_type: str
    property_class: str
    offset: int
    is_object_ptr: bool = False
    pointee_type: str = ""


@dataclass
class ParamInfo:
    name: str
    cpp_type: str
    property_class: str


@dataclass
class FunctionInfo:
    name: str
    return_type: str
    return_property_class: str
    params: list[ParamInfo] = field(default_factory=list)

    @property
    def has_params_struct(self) -> bool:
        return bool(self.params) or self.return_type != "void"


@dataclass
class ClassInfo:
    name: str
    source_file: Path
    include_path: str
    properties: list[PropertyInfo] = field(default_factory=list)
    functions: list[FunctionInfo] = field(default_factory=list)


# ── token-based macro detection ──────────────────────────────


def _get_tokens_before_cursor(tu, cursor, lookback_lines=5):
    start_line = max(1, cursor.location.line - lookback_lines)
    extent = tu.get_extent(
        cursor.location.file.name,
        ((start_line, 1), (cursor.location.line, cursor.location.column)),
    )
    return list(tu.get_tokens(extent=extent))


def _is_annotated(tu, cursor, macro_name):
    tokens = _get_tokens_before_cursor(tu, cursor)
    for i, tok in enumerate(reversed(tokens)):
        if tok.spelling == macro_name:
            return True
        if i > 10:
            break
    return False


# ── AST walking ──────────────────────────────────────────────


def _parse_function(tu, method_cursor, class_name):
    ret_type = method_cursor.result_type
    ret_spelling = ret_type.spelling
    is_void = ret_spelling == "void"

    ret_prop_class = ""
    if not is_void:
        resolved = resolve_type(ret_type, "returnValue", class_name)
        if resolved:
            ret_prop_class = resolved[0]
        else:
            ret_prop_class = ""

    params: list[ParamInfo] = []
    for child in method_cursor.get_children():
        if child.kind == ci.CursorKind.PARM_DECL:
            p_resolved = resolve_type(child.type, child.spelling, class_name)
            if p_resolved is None:
                continue
            params.append(ParamInfo(
                name=child.spelling,
                cpp_type=child.type.spelling,
                property_class=p_resolved[0],
            ))

    return FunctionInfo(
        name=method_cursor.spelling,
        return_type=ret_spelling,
        return_property_class=ret_prop_class,
        params=params,
    )


def _parse_class(tu, class_cursor, source_file, include_path):
    class_name = class_cursor.spelling
    info = ClassInfo(
        name=class_name,
        source_file=source_file,
        include_path=include_path,
    )

    for child in class_cursor.get_children():
        if child.kind == ci.CursorKind.FIELD_DECL:
            if not _is_annotated(tu, child, "DPROPERTY"):
                continue
            resolved = resolve_type(child.type, child.spelling, class_name)
            if resolved is None:
                continue
            prop_class, is_obj_ptr, pointee = resolved
            offset_bits = class_cursor.type.get_offset(child.spelling)
            offset_bytes = offset_bits // 8 if offset_bits >= 0 else -1
            info.properties.append(PropertyInfo(
                name=child.spelling,
                cpp_type=child.type.spelling,
                property_class=prop_class,
                offset=offset_bytes,
                is_object_ptr=is_obj_ptr,
                pointee_type=pointee,
            ))

        elif child.kind == ci.CursorKind.CXX_METHOD:
            if not _is_annotated(tu, child, "DFUNCTION"):
                continue
            info.functions.append(
                _parse_function(tu, child, class_name)
            )

    return info


# ── public API ───────────────────────────────────────────────


def parse_header(
    file_path: Path,
    input_dir: Path,
    extra_include_dirs: list[Path] | None = None,
) -> list[ClassInfo]:
    _ensure_configured()

    file_path = file_path.resolve()
    input_dir = input_dir.resolve()
    engine_include_root = input_dir.parent  # e.g. Engine/

    args = ["-std=c++20", "-x", "c++"]
    args.append(f"-I{input_dir}")
    args.append(f"-I{engine_include_root}")
    for d in (extra_include_dirs or []):
        args.append(f"-I{Path(d).resolve()}")

    index = ci.Index.create()
    tu = index.parse(str(file_path), args=args)

    errors = [
        d for d in tu.diagnostics
        if d.severity >= ci.Diagnostic.Error
    ]
    if errors:
        for e in errors:
            print(f"  {e}", file=sys.stderr)

    file_str = str(file_path)

    rel = file_path.relative_to(engine_include_root)
    parent_part = rel.parent.as_posix()
    include_path = (parent_part + "/") if parent_part != "." else ""

    results: list[ClassInfo] = []
    for cursor in tu.cursor.walk_preorder():
        if cursor.location.file is None:
            continue
        if str(Path(cursor.location.file.name).resolve()) != file_str:
            continue
        if cursor.kind not in (ci.CursorKind.CLASS_DECL, ci.CursorKind.STRUCT_DECL):
            continue
        if not cursor.is_definition():
            continue
        if not _is_annotated(tu, cursor, "DCLASS"):
            continue

        results.append(_parse_class(tu, cursor, file_path, include_path))

    return results
