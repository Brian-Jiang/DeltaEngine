from __future__ import annotations

import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

import clang.cindex as ci

from type_resolver import normalize_type_spelling, resolve_type, resolve_type_spelling

# Use canonical (underlying) type for emission when type is a typedef/using alias
# so param struct and thunk use e.g. __m128 instead of DirectX::XMVECTOR.
def _type_spelling_for_emission(clang_type) -> str:
    """Return type spelling to emit in param struct and thunk.
    Strips references, const qualifiers, and typedef/alias to produce a value type."""
    t = clang_type
    if t.kind == ci.TypeKind.LVALUEREFERENCE or t.kind == ci.TypeKind.RVALUEREFERENCE:
        t = t.get_pointee()
    if t.kind == ci.TypeKind.TYPEDEF or t.kind == ci.TypeKind.ELABORATED:
        t = t.get_canonical()
    return normalize_type_spelling(t.spelling)

TOOL_DIR = Path(__file__).resolve().parent
TOOLS_DIR = TOOL_DIR.parent
LIB_PATH = TOOLS_DIR / "Clang"

_configured = False

# Strip #include lines so clang never touches the filesystem for dependencies.
_INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"].*?[>"]', re.MULTILINE)

# Capture full #include directives for re-emission (exact line including quote/angle).
_INCLUDE_CAPTURE_RE = re.compile(r'^\s*#\s*include\s+[<"][^">]+[">]', re.MULTILINE)

# Preamble: stubs and forward declarations so stripped headers parse well enough
# for DCLASS/DSTRUCT/DPROPERTY/DFUNCTION extraction. We tolerate unresolved-type errors.
_PREAMBLE = r"""
// --- primitive typedefs (cstdint-style) ---
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;

// --- Windows / D3D12 minimal stubs ---
typedef unsigned int UINT;
typedef int DXGI_FORMAT;
struct IDxcBlob;
struct D3D12_INPUT_ELEMENT_DESC { unsigned int dummy; };
struct CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC { int dummy; };
struct CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL { int dummy; };
namespace Microsoft { namespace WRL { template<typename T> class ComPtr { T* p; }; }}

// --- DirectX stubs ---
namespace DirectX {
namespace SimpleMath {
struct Vector3 { float x, y, z; };
struct Quaternion { float x, y, z, w; };
}
struct XMFLOAT4X4 { float m[4][4]; };
struct XMFLOAT4 { float x, y, z, w; };
struct XMMATRIX { float m[4][4]; };
struct __declspec(align(16)) XMVECTOR { float f[4]; };
typedef XMFLOAT4 XMFLOAT2;
typedef XMFLOAT4 XMFLOAT3;
struct TexMetadata {};
class ScratchImage {};
}

// --- Assimp stubs ---
struct aiNode {};
struct aiScene {};
struct aiMesh {};
struct aiMaterial {};

// --- std stubs ---
namespace std {
template<typename T> struct char_traits {};
template<typename C, typename T> class basic_string {};
typedef basic_string<char, char_traits<char>> string;
typedef basic_string<wchar_t, char_traits<wchar_t>> wstring;
template<typename T> class shared_ptr { T* p; };
template<typename T> class weak_ptr { T* p; };
template<typename T, typename A = void> class vector {};
template<typename T> class enable_shared_from_this {};
}

// --- DeltaEngine forward declarations ---
namespace DeltaEngine {
class DObject;
class DClass;
class DStruct;
class GameObject;
class DComponent;
class SceneComponent;
class DWorld;
class Camera;
class Renderer;
class LightComponent;
class DirectionalLight;
class PointLight;
class SpotLight;
class DTexture;
class DMaterial;
class DMesh;
class DShader;
class TestComponent;
class TestComponent2;
struct DXGraphicsContext;
class CameraRenderProxy;
class DirectionalLightRenderProxy;
class PointLightRenderProxy;
class SpotLightRenderProxy;
class MeshRenderProxy;
class EngineMain;
struct MeshRendererSettings {};
struct Vertex { float x; };
}

// --- Reflection macros (redefine to annotate form for libclang) ---
#define DCLASS(...)
#define DSTRUCT(...)
#define DPROPERTY(...)
#define DFUNCTION(...)
#define DGENERATED_BODY(ClassName)
#define DGENERATED_BODY_STRUCT(StructName)

#define DELTA_ENGINE_NS_BEGIN  namespace DeltaEngine {
#define DELTA_ENGINE_NS_END    }
#define DELTAENGINE_API
"""

# Line count of the preamble; declarations at line <= this are from the preamble, not the source file.
_PREAMBLE_LINE_COUNT = len(_PREAMBLE.splitlines())


def _to_source_line(parsed_line: int) -> int:
    return max(1, parsed_line - _PREAMBLE_LINE_COUNT)


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
    line: int = 0
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
    overload_index: int = 1  # 1-based; _2, _3, ... for overloads
    line: int = 0
    macro_line: int = 0
    is_inline: bool = False
    is_template: bool = False

    @property
    def has_params_struct(self) -> bool:
        return bool(self.params) or self.return_type != "void"


@dataclass
class ConstructorInfo:
    signature: str
    line: int
    is_default: bool


@dataclass
class ClassInfo:
    name: str
    source_file: Path
    include_path: str
    line: int = 0
    base_name: str = ""
    declared_super_name: str = ""
    properties: list[PropertyInfo] = field(default_factory=list)
    functions: list[FunctionInfo] = field(default_factory=list)
    constructors: list[ConstructorInfo] = field(default_factory=list)
    template_dfunction_macro_lines: list[int] = field(default_factory=list)
    is_struct: bool = False
    is_abstract: bool = False


@dataclass
class ForwardDeclInfo:
    kind: str           # "class", "struct", "template_class"
    name: str
    namespaces: tuple[str, ...]  # ("DeltaEngine",) or () for global


@dataclass
class ParseResult:
    """Result of parsing one header: reflected classes plus file-level data."""
    classes: list[ClassInfo]
    source_includes: list[str]
    forward_decls: list[ForwardDeclInfo]
    warnings: list[str] = field(default_factory=list)


def _format_warning(file_path: Path, line: int, message: str) -> str:
    return f"WARNING: [{file_path.as_posix()}:{line}] {message}"


# ── source includes and forward decls ────────────────────────


def _collect_source_includes(raw: str, file_path: Path) -> list[str]:
    """Collect #include lines from raw source; exclude self .generated.h; dedupe preserving order."""
    matches = _INCLUDE_CAPTURE_RE.findall(raw)
    stem = file_path.stem
    self_generated = f"{stem}.generated.h"
    result: list[str] = []
    for m in matches:
        line = m.strip()
        inner = re.search(r'#include\s+[<"]([^">]+)[">]', line)
        if inner:
            path = inner.group(1)
            tail = path.replace("\\", "/").split("/")[-1]
            if tail == self_generated:
                continue
        result.append(line)
    return list(dict.fromkeys(result))


def _is_anonymous_or_invalid(spelling: str) -> bool:
    """Return True if the type spelling should not be forward-declared."""
    if not spelling or not spelling.strip():
        return True
    s = spelling.strip()
    if s.startswith("(anonymous") or s.startswith("(unnamed"):
        return True
    return False


def _get_namespace_chain(cursor) -> tuple[str, ...]:
    """Walk semantic parents to build the namespace chain for a cursor."""
    namespaces: list[str] = []
    parent = cursor.semantic_parent
    while parent and parent.kind != ci.CursorKind.TRANSLATION_UNIT:
        if parent.kind == ci.CursorKind.NAMESPACE:
            namespaces.append(parent.spelling)
        parent = parent.semantic_parent
    namespaces.reverse()
    return tuple(namespaces)


def _collect_forward_decls(tu, file_str: str, source_line_start: int) -> list[ForwardDeclInfo]:
    """Walk AST and collect forward declaration info for every class/struct/class_template
    declared in the source file, including their namespace chain.
    Cursors at line <= source_line_start are from the preamble and are excluded."""
    seen: set[tuple[str, str, tuple[str, ...]]] = set()
    decls: list[ForwardDeclInfo] = []
    for cursor in tu.cursor.walk_preorder():
        if cursor.location.file is None:
            continue
        if str(Path(cursor.location.file.name).resolve()) != file_str:
            continue
        if cursor.location.line <= source_line_start:
            continue
        kind = None
        if cursor.kind == ci.CursorKind.CLASS_DECL:
            kind = "class"
        elif cursor.kind == ci.CursorKind.STRUCT_DECL:
            kind = "struct"
        elif cursor.kind == ci.CursorKind.CLASS_TEMPLATE:
            kind = "template_class"
        if kind is None:
            continue
        spelling = cursor.spelling or ""
        if _is_anonymous_or_invalid(spelling):
            continue
        ns_chain = _get_namespace_chain(cursor)
        key = (kind, spelling, ns_chain)
        if key not in seen:
            seen.add(key)
            decls.append(ForwardDeclInfo(kind=kind, name=spelling, namespaces=ns_chain))
    return decls


# ── token-based macro detection ──────────────────────────────


def _get_tokens_before_cursor(tu, cursor, lookback_lines=5):
    start_line = max(1, cursor.location.line - lookback_lines)
    extent = tu.get_extent(
        cursor.location.file.name,
        ((start_line, 1), (cursor.location.line, cursor.location.column)),
    )
    return list(tu.get_tokens(extent=extent))


def _is_annotated(tu, cursor, macro_name, *, max_line_gap: int = 2):
    tokens = _get_tokens_before_cursor(tu, cursor)
    for tok in reversed(tokens):
        if tok.spelling == macro_name and (cursor.location.line - tok.location.line) <= max_line_gap:
            return True
    return False


def _extract_macro_args(tu, cursor, macro_name, *, max_line_gap: int = 20) -> str | None:
    """Extract the argument string from a macro invocation like DCLASS(abstract).
    Returns the text between parentheses, or None if not found."""
    tokens = _get_tokens_before_cursor(tu, cursor)
    for i, tok in enumerate(reversed(tokens)):
        if tok.spelling == macro_name and (cursor.location.line - tok.location.line) <= max_line_gap:
            # Collect tokens forward from macro position to find (...)
            macro_idx = len(tokens) - 1 - i
            if macro_idx + 1 < len(tokens) and tokens[macro_idx + 1].spelling == "(":
                depth = 0
                arg_tokens = []
                for t in tokens[macro_idx + 1:]:
                    if t.spelling == "(":
                        depth += 1
                        if depth > 1:
                            arg_tokens.append(t.spelling)
                    elif t.spelling == ")":
                        depth -= 1
                        if depth == 0:
                            return "".join(arg_tokens).strip()
                        arg_tokens.append(t.spelling)
                    else:
                        arg_tokens.append(t.spelling)
            return ""
    return None


def _collect_dfunction_lines(tu, class_cursor) -> list[int]:
    """Collect line numbers of all DFUNCTION tokens within the class body."""
    tokens = list(tu.get_tokens(extent=class_cursor.extent))
    lines: list[int] = []
    for tok in tokens:
        if tok.spelling == "DFUNCTION":
            lines.append(tok.location.line)
    return lines


def _is_std_enable_shared_from_this(base_name: str) -> bool:
    n = base_name.replace(" ", "")
    return (
        n.startswith("std::enable_shared_from_this<")
        or n.startswith("enable_shared_from_this<")
    )


def _is_std_base(base_name: str) -> bool:
    return base_name.strip().startswith("std::")


def _resolve_filtered_super_name(base_specifiers: list[str]) -> str:
    for base in base_specifiers:
        normalized = normalize_type_spelling(base)
        if _is_std_enable_shared_from_this(normalized):
            continue
        if _is_std_base(normalized):
            continue
        return normalized
    return ""


def _extract_declared_field_type(source_lines: list[str], line: int, field_name: str) -> str:
    for probe in range(line - 1, line + 3):
        idx = probe - 1
        if idx < 0 or idx >= len(source_lines):
            continue
        line_text = source_lines[idx]
        # Strip trailing comment and semicolon.
        line_text = line_text.split("//", 1)[0].strip().rstrip(";").strip()
        if not line_text:
            continue
        m = re.match(rf"(?P<type>.+?)\s+{re.escape(field_name)}\s*$", line_text)
        if m:
            return m.group("type").strip()
    return ""


def _method_is_inline(tu, method_cursor) -> bool:
    try:
        if method_cursor.is_function_inlined():
            return True
    except Exception:
        pass

    tokens = list(tu.get_tokens(extent=method_cursor.extent))
    return any(tok.spelling == "inline" for tok in tokens)


def _extract_constructor_signature(class_name: str, ctor_cursor) -> str:
    params: list[str] = []
    for child in ctor_cursor.get_children():
        if child.kind == ci.CursorKind.PARM_DECL:
            ptype = _type_spelling_for_emission(child.type)
            pname = child.spelling or "arg"
            params.append(f"{ptype} {pname}")
    return f"{class_name}({', '.join(params)})"


def _find_next_annotated_method_line(
    dfunction_lines: list[int],
    consumed_dfunction_lines: set[int],
    target_line: int,
) -> int | None:
    for dl in dfunction_lines:
        if dl in consumed_dfunction_lines:
            continue
        if dl < target_line and (target_line - dl) <= 2:
            consumed_dfunction_lines.add(dl)
            return dl
    return None


# ── AST walking ──────────────────────────────────────────────


def _parse_function(tu, method_cursor, class_name):
    ret_type = method_cursor.result_type
    ret_spelling = ret_type.spelling
    is_void = ret_spelling == "void"
    ret_emission = _type_spelling_for_emission(ret_type) if not is_void else "void"

    ret_prop_class = ""
    if not is_void:
        resolved = resolve_type(ret_type, "returnValue", class_name)
        if resolved:
            ret_prop_class = resolved[0]
        else:
            ret_prop_class = ""

    params: list[ParamInfo] = []
    expected_param_count = sum(
        1 for child in method_cursor.get_children()
        if child.kind == ci.CursorKind.PARM_DECL
    )
    for child in method_cursor.get_children():
        if child.kind == ci.CursorKind.PARM_DECL:
            p_resolved = resolve_type(child.type, child.spelling, class_name)
            if p_resolved is None:
                continue
            params.append(ParamInfo(
                name=child.spelling,
                cpp_type=_type_spelling_for_emission(child.type),
                property_class=p_resolved[0],
            ))

    if len(params) != expected_param_count:
        return None

    return FunctionInfo(
        name=method_cursor.spelling,
        return_type=ret_emission if not is_void else "void",
        return_property_class=ret_prop_class,
        params=params,
        line=method_cursor.location.line,
        is_inline=_method_is_inline(tu, method_cursor),
    )


def _parse_class(
    tu,
    class_cursor,
    source_file,
    include_path,
    warnings: list[str],
    source_lines: list[str],
    *,
    is_struct=False,
    is_abstract=False,
):
    class_name = class_cursor.spelling
    info = ClassInfo(
        name=class_name,
        source_file=source_file,
        include_path=include_path,
        line=_to_source_line(class_cursor.location.line),
        is_struct=is_struct,
        is_abstract=is_abstract,
    )

    # Pre-collect DFUNCTION macro line numbers for scope-correct matching
    dfunction_lines = _collect_dfunction_lines(tu, class_cursor)
    consumed_dfunction_lines: set[int] = set()
    base_specifiers: list[str] = []

    for child in class_cursor.get_children():
        if child.kind == ci.CursorKind.CXX_BASE_SPECIFIER:
            base_specifiers.append(child.spelling)
            continue

        if child.kind == ci.CursorKind.FIELD_DECL:
            if not _is_annotated(tu, child, "DPROPERTY", max_line_gap=2):
                continue
            source_line = _to_source_line(child.location.line)
            declared_cpp_type = _extract_declared_field_type(source_lines, source_line, child.spelling)
            resolved = None
            if declared_cpp_type:
                resolved = resolve_type_spelling(declared_cpp_type, child.spelling, class_name)
            if resolved is None:
                resolved = resolve_type(child.type, child.spelling, class_name)
            if resolved is None:
                unknown_type = (
                    normalize_type_spelling(declared_cpp_type)
                    if declared_cpp_type
                    else normalize_type_spelling(child.type.spelling)
                )
                warnings.append(
                    _format_warning(
                        source_file,
                        source_line,
                        (
                            f"Property '{child.spelling}' of type '{unknown_type}' in "
                            f"'{class_name}' has no known property type mapping — skipped."
                        ),
                    )
                )
                continue
            prop_class, is_obj_ptr, pointee = resolved
            offset_bits = class_cursor.type.get_offset(child.spelling)
            offset_bytes = offset_bits // 8 if offset_bits >= 0 else -1
            info.properties.append(PropertyInfo(
                name=child.spelling,
                cpp_type=normalize_type_spelling(declared_cpp_type or child.type.spelling),
                property_class=prop_class,
                offset=offset_bytes,
                line=source_line,
                is_object_ptr=is_obj_ptr,
                pointee_type=pointee,
            ))

        elif child.kind == ci.CursorKind.CONSTRUCTOR:
            if is_struct:
                continue
            is_default_ctor = False
            try:
                is_default_ctor = child.is_default_constructor()
            except Exception:
                is_default_ctor = False

            signature = _extract_constructor_signature(class_name, child)
            info.constructors.append(ConstructorInfo(
                signature=signature,
                line=_to_source_line(child.location.line),
                is_default=is_default_ctor,
            ))

            if not is_default_ctor:
                warnings.append(
                    _format_warning(
                        source_file,
                        _to_source_line(child.location.line),
                        (
                            f"'{class_name}' has non-default constructor '{signature}' — "
                            "DObject-derived classes should use Initialize() pattern instead. "
                            "Consider deleting this constructor."
                        ),
                    )
                )

        elif child.kind == ci.CursorKind.CXX_METHOD:
            if is_struct:
                continue

            method_line = child.location.line
            macro_line = _find_next_annotated_method_line(dfunction_lines, consumed_dfunction_lines, method_line)
            if macro_line is None:
                continue

            fn = _parse_function(tu, child, class_name)
            if fn is None:
                continue
            fn.macro_line = _to_source_line(macro_line)
            if fn.is_inline:
                warnings.append(
                    _format_warning(
                        source_file,
                        _to_source_line(child.location.line),
                        (
                            f"DFUNCTION on '{class_name}::{fn.name}' is inline — "
                            "definition will be moved to .cpp."
                        ),
                    )
                )
            fn.line = _to_source_line(fn.line)
            same_name_count = sum(1 for f in info.functions if f.name == fn.name)
            fn.overload_index = same_name_count + 1
            info.functions.append(fn)

        elif child.kind == ci.CursorKind.FUNCTION_TEMPLATE:
            if is_struct:
                continue
            method_line = child.location.line
            macro_line = _find_next_annotated_method_line(dfunction_lines, consumed_dfunction_lines, method_line)
            if macro_line is None:
                continue
            info.template_dfunction_macro_lines.append(_to_source_line(macro_line))
            fn_name = child.spelling or "<template>"
            warnings.append(
                _format_warning(
                    source_file,
                    _to_source_line(child.location.line),
                    (
                        f"DFUNCTION on '{class_name}::{fn_name}' is a template — "
                        "DFUNCTION will be removed, function will not be reflected."
                    ),
                )
            )

    info.declared_super_name = _resolve_filtered_super_name(base_specifiers)
    info.base_name = info.declared_super_name

    return info


# ── public API ───────────────────────────────────────────────


def parse_header(
    file_path: Path,
    input_dir: Path,
    extra_include_dirs: list[Path] | None = None,
) -> ParseResult:
    _ensure_configured()

    file_path = file_path.resolve()
    input_dir = input_dir.resolve()
    engine_include_root = input_dir.parent  # e.g. Engine/

    raw = file_path.read_text(encoding="utf-8", errors="replace")
    source_lines = raw.splitlines()
    source_includes = _collect_source_includes(raw, file_path)
    stripped_source = _PREAMBLE + _INCLUDE_RE.sub("", raw)

    args = ["-std=c++23", "-x", "c++", "-w", "-ferror-limit=0"]
    parse_options = (
        ci.TranslationUnit.PARSE_SKIP_FUNCTION_BODIES
        | ci.TranslationUnit.PARSE_INCOMPLETE
    )

    index = ci.Index.create()
    tu = index.parse(
        str(file_path),
        args=args,
        unsaved_files=[(str(file_path), stripped_source)],
        options=parse_options,
    )

    fatals = [d for d in tu.diagnostics if d.severity == ci.Diagnostic.Fatal]
    if fatals:
        for d in fatals:
            print(f"  {d}", file=sys.stderr)
        raise RuntimeError(
            f"Fatal diagnostic(s) parsing {file_path.name}; cannot continue"
        )

    file_str = str(file_path)
    forward_decls = _collect_forward_decls(tu, file_str, _PREAMBLE_LINE_COUNT)

    rel = file_path.relative_to(engine_include_root)
    parent_part = rel.parent.as_posix()
    include_path = (parent_part + "/") if parent_part != "." else ""

    results: list[ClassInfo] = []
    warnings: list[str] = []
    for cursor in tu.cursor.walk_preorder():
        if cursor.location.file is None:
            continue
        if str(Path(cursor.location.file.name).resolve()) != file_str:
            continue
        if cursor.kind not in (ci.CursorKind.CLASS_DECL, ci.CursorKind.STRUCT_DECL):
            continue
        if not cursor.is_definition():
            continue

        is_dclass = _is_annotated(tu, cursor, "DCLASS", max_line_gap=20)
        is_dstruct = _is_annotated(tu, cursor, "DSTRUCT", max_line_gap=20)

        if not is_dclass and not is_dstruct:
            continue

        is_abstract = False
        if is_dclass:
            macro_args = _extract_macro_args(tu, cursor, "DCLASS", max_line_gap=20)
            if macro_args and "abstract" in macro_args:
                is_abstract = True
            if not is_abstract:
                is_abstract = cursor.is_abstract_record()

        results.append(_parse_class(
            tu, cursor, file_path, include_path,
            warnings,
            source_lines,
            is_struct=is_dstruct,
            is_abstract=is_abstract,
        ))

    return ParseResult(
        classes=results,
        source_includes=source_includes,
        forward_decls=forward_decls,
        warnings=warnings,
    )
