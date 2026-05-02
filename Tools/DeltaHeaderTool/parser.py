from __future__ import annotations

import platform
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

import clang.cindex as ci

from macro_utils import get_tokens_before_cursor as _get_tokens_before_cursor
from macro_utils import is_annotated as _is_annotated
from type_resolver import resolve_type, resolve_type_from_string
from diagnostics import DiagnosticCollector

# Use canonical (underlying) type for emission when type is a typedef/using alias
# so param struct and thunk use e.g. __m128 instead of DirectX::XMVECTOR.
_BASIC_STRING_NORMALIZE = [
    (re.compile(r"std::basic_string<char(?:,\s*std::char_traits<char>(?:,\s*std::allocator<char>)?)?>"), "std::string"),
    (re.compile(r"std::basic_string<wchar_t(?:,\s*std::char_traits<wchar_t>(?:,\s*std::allocator<wchar_t>)?)?>"), "std::wstring"),
]

def _type_spelling_for_emission(clang_type) -> str:
    """Return type spelling to emit in param struct and thunk.
    Strips references, const qualifiers, and typedef/alias to produce a value type."""
    t = clang_type
    if t.kind == ci.TypeKind.LVALUEREFERENCE or t.kind == ci.TypeKind.RVALUEREFERENCE:
        t = t.get_pointee()
    if t.kind == ci.TypeKind.TYPEDEF or t.kind == ci.TypeKind.ELABORATED:
        t = t.get_canonical()
    spelling = t.spelling
    if spelling.startswith("const "):
        spelling = spelling[6:]
    for pattern, replacement in _BASIC_STRING_NORMALIZE:
        spelling = pattern.sub(replacement, spelling)
    return spelling

TOOL_DIR = Path(__file__).resolve().parent
TOOLS_DIR = TOOL_DIR.parent
LIB_PATH = TOOLS_DIR / "Clang"

_configured = False


def libclang_library_path() -> Path:
    """Path to the libclang shared library expected by this repo."""
    if platform.system() == "Windows":
        return LIB_PATH / "libclang.dll"
    return LIB_PATH / "libclang.so"


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
namespace filesystem {
class path {};
}
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
struct MeshRendererSettings;
struct Vertex { float x; };
struct TBulkData;
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


def _ensure_configured():
    global _configured
    if not _configured:
        lib = libclang_library_path()
        if not lib.exists():
            raise RuntimeError(
                f"libclang not found at {lib}; place the LLVM libclang binary in Tools/Clang/."
            )
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
    metadata: dict = field(default_factory=dict)
    is_vector: bool = False
    inner_cpp_type: str = ""
    inner_property_class: str = ""
    inner_is_object_ptr: bool = False
    inner_pointee_type: str = ""
    is_dstruct: bool = False
    dstruct_type_name: str = ""
    editor_only: bool = False
    hide_in_details: bool = False


@dataclass
class ParamInfo:
    name: str
    cpp_type: str
    property_class: str
    is_vector: bool = False
    inner_cpp_type: str = ""
    inner_property_class: str = ""
    inner_is_object_ptr: bool = False
    inner_pointee_type: str = ""


@dataclass
class FunctionInfo:
    name: str
    return_type: str
    return_property_class: str
    params: list[ParamInfo] = field(default_factory=list)
    overload_index: int = 1  # 1-based; _2, _3, ... for overloads
    return_is_vector: bool = False
    return_inner_cpp_type: str = ""
    return_inner_property_class: str = ""
    return_inner_is_object_ptr: bool = False
    return_inner_pointee_type: str = ""
    metadata: dict[str, str] = field(default_factory=dict)

    @property
    def has_params_struct(self) -> bool:
        return bool(self.params) or self.return_type != "void"


@dataclass
class ClassInfo:
    name: str
    source_file: Path
    include_path: str
    base_name: str = ""
    properties: list[PropertyInfo] = field(default_factory=list)
    functions: list[FunctionInfo] = field(default_factory=list)
    is_struct: bool = False
    is_abstract: bool = False
    metadata: dict[str, str] = field(default_factory=dict)


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
    diagnostics: DiagnosticCollector = field(default_factory=DiagnosticCollector)


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


def _base_class_name_from_cursor(cursor) -> str:
    """Extract unqualified base class name from CXX_BASE_SPECIFIER cursor.
    cursor.spelling is often empty for base specifiers; use type.spelling as fallback."""
    name = (cursor.spelling or "").strip()
    if not name:
        # libclang often leaves spelling empty for base specifiers; use type
        t = cursor.type
        if t and t.kind != ci.TypeKind.INVALID:
            name = (t.spelling or "").strip()
    if not name:
        return ""
    # Skip mixin bases like enable_shared_from_this
    if "enable_shared_from_this" in name:
        return ""
    # Strip "class " and "struct " prefixes
    for prefix in ("class ", "struct "):
        if name.startswith(prefix):
            name = name[len(prefix):].strip()
    # Strip namespace qualifiers: DeltaEngine::DObject -> DObject
    if "::" in name:
        name = name.rsplit("::", 1)[-1]
    return name


def _extract_dproperty_fields_from_source(source: str, class_name: str, class_start_line: int, class_end_line: int) -> list[tuple[str, str, str]]:
    """Text-scan for DPROPERTY fields that may be missing from the AST (e.g. std::string when using stub types).
    Returns list of (field_name, type_str, args_str) for fields preceded by DPROPERTY()."""
    lines = source.splitlines()
    result: list[tuple[str, str, str]] = []
    i = class_start_line - 1  # 0-based
    while i < len(lines) and i < class_end_line:
        line = lines[i]
        if "DPROPERTY" in line and re.search(r"DPROPERTY\s*\(", line):
            args_match = re.search(r"DPROPERTY\s*\((.*)\)", line)
            args_str = args_match.group(1).strip() if args_match else ""
            j = i + 1
            while j < len(lines) and j < class_end_line:
                decl_line = lines[j].strip()
                if not decl_line or decl_line.startswith("//"):
                    j += 1
                    continue
                m = re.match(r"^(.+?)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*[;=]", decl_line)
                if m:
                    type_str = m.group(1).strip()
                    field_name = m.group(2)
                    result.append((field_name, type_str, args_str))
                break
            i = j
        i += 1
    return result


def _extract_base_from_source(source: str, class_name: str, start_line: int) -> str:
    """Extract first reflected base class from class declaration in source.
    Used when libclang does not provide CXX_BASE_SPECIFIER (e.g. with stripped/incomplete parse)."""
    lines = source.splitlines()
    if start_line < 1 or start_line > len(lines):
        return ""
    # Collect declaration lines until we hit {
    decl_parts: list[str] = []
    for i in range(start_line - 1, len(lines)):
        line = lines[i]
        decl_parts.append(line)
        if "{" in line:
            break
    decl = " ".join(decl_parts)
    # Match ": base_specifiers" - after class/struct Name :
    m = re.search(rf"\b(?:class|struct)\s+(?:\w+\s+)*{re.escape(class_name)}\s*:\s*(.+?)(?:\{{|$)", decl, re.DOTALL)
    if not m:
        return ""
    bases_str = m.group(1).strip()
    # Split by comma, respecting angle brackets
    bases: list[str] = []
    depth = 0
    start = 0
    for i, c in enumerate(bases_str + ","):
        if c == "<":
            depth += 1
        elif c == ">":
            depth -= 1
        elif c == "," and depth == 0:
            bases.append(bases_str[start:i].strip())
            start = i + 1
    for base in bases:
        # Strip access specifier
        for prefix in ("public ", "protected ", "private "):
            if base.startswith(prefix):
                base = base[len(prefix):].strip()
        if "enable_shared_from_this" in base:
            continue
        # Get unqualified name: DeltaEngine::DObject -> DObject
        if "::" in base:
            base = base.rsplit("::", 1)[-1]
        # Strip template args for the name: Foo<T> -> Foo (we want the base class name)
        if "<" in base:
            base = base[: base.index("<")].strip()
        if base:
            return base
    return ""


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


def _extract_macro_args(tu, cursor, macro_name) -> str | None:
    """Extract the argument string from a macro invocation like DCLASS(abstract).
    Returns the text between parentheses, or None if not found."""
    tokens = _get_tokens_before_cursor(tu, cursor)
    for i, tok in enumerate(reversed(tokens)):
        if tok.spelling == macro_name:
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
        if tok.spelling == ";":
            break  # crossed a previous declaration boundary; stop searching
        if i > 64:
            break
    return None


def _parse_meta_kv(args_str: str) -> dict[str, str]:
    """Extract key=value pairs from meta=(...) in any DCLASS/DSTRUCT/DPROPERTY/DFUNCTION
    argument string. Keys must consist of letters and digits only.
    e.g. 'meta=(UIType="Color", Category="Shadows")' -> {'UIType': 'Color', 'Category': 'Shadows'}
    """
    if not args_str:
        return {}
    m = re.search(r'meta\s*=\s*\(([^)]*)\)', args_str)
    if not m:
        return {}
    content = m.group(1)
    result = {}
    for kv in re.finditer(r'(?<!\w)([A-Za-z0-9]+)\s*=\s*"([^"]*)"', content):
        result[kv.group(1)] = kv.group(2)
    return result


# Backwards-compatible alias.
_parse_dproperty_meta = _parse_meta_kv


def _collect_dfunction_lines(tu, class_cursor) -> list[tuple[int, str]]:
    """Collect (line, args_str) of all DFUNCTION macro invocations within the class body."""
    tokens = list(tu.get_tokens(extent=class_cursor.extent))
    result: list[tuple[int, str]] = []
    i = 0
    while i < len(tokens):
        tok = tokens[i]
        if tok.spelling == "DFUNCTION":
            line = tok.location.line
            args_str = ""
            if i + 1 < len(tokens) and tokens[i + 1].spelling == "(":
                depth = 0
                arg_tokens: list[str] = []
                j = i + 1
                while j < len(tokens):
                    t = tokens[j]
                    if t.spelling == "(":
                        depth += 1
                        if depth > 1:
                            arg_tokens.append(t.spelling)
                    elif t.spelling == ")":
                        depth -= 1
                        if depth == 0:
                            j += 1
                            break
                        arg_tokens.append(t.spelling)
                    else:
                        arg_tokens.append(t.spelling)
                    j += 1
                args_str = "".join(arg_tokens).strip()
                i = j
                result.append((line, args_str))
                continue
            result.append((line, args_str))
        i += 1
    return result


def _parse_dfunction_meta(args_str: str) -> dict[str, str]:
    """Parse DFUNCTION(...) arguments into a metadata dict.

    Supports bare identifiers (treated as flags with value "true") and key="value" pairs,
    plus meta=(key="value", ...) nested form for forward compatibility with DPROPERTY syntax.

    Examples:
      'ShowAsButton'                 -> {'ShowAsButton': 'true'}
      'ShowAsButton, Category="Dbg"' -> {'ShowAsButton': 'true', 'Category': 'Dbg'}
      'meta=(UIType="Color")'        -> {'UIType': 'Color'}
    """
    if not args_str:
        return {}
    result: dict[str, str] = {}

    # Strip and capture meta=(...) block first so it doesn't interfere with outer splitting.
    remainder = args_str
    meta_match = re.search(r'meta\s*=\s*\(([^)]*)\)', remainder)
    if meta_match:
        inner = meta_match.group(1)
        for kv in re.finditer(r'(?<!\w)([A-Za-z0-9]+)\s*=\s*"([^"]*)"', inner):
            result[kv.group(1)] = kv.group(2)
        remainder = remainder[:meta_match.start()] + remainder[meta_match.end():]

    # Split remaining top-level args by commas.
    for part in remainder.split(","):
        part = part.strip()
        if not part:
            continue
        kv = re.match(r'(?<!\w)([A-Za-z0-9]+)\s*=\s*"([^"]*)"\s*$', part)
        if kv:
            result[kv.group(1)] = kv.group(2)
            continue
        bare = re.match(r'(\w+)\s*$', part)
        if bare:
            result[bare.group(1)] = "true"
    return result


# ── AST walking ──────────────────────────────────────────────


def _parse_function(tu, method_cursor, class_name, diag=None, source_file=""):
    ret_type = method_cursor.result_type
    ret_spelling = ret_type.spelling
    is_void = ret_spelling == "void"
    ret_emission = _type_spelling_for_emission(ret_type) if not is_void else "void"

    ret_prop_class = ""
    ret_is_vec = False
    ret_inner_cpp = ""
    ret_inner_prop = ""
    ret_inner_obj = False
    ret_inner_pt = ""
    if not is_void:
        resolved = resolve_type(ret_type, "returnValue", class_name,
                                diag=diag, source_file=source_file,
                                line=method_cursor.location.line - _PREAMBLE_LINE_COUNT)
        if resolved:
            ret_prop_class = resolved[0]
            if len(resolved) >= 5:
                ret_is_vec = True
                ret_inner_cpp = resolved[3]
                ret_inner_prop = resolved[4]
                ret_inner_obj = resolved[5]
                ret_inner_pt = resolved[6]

    params: list[ParamInfo] = []
    expected_param_count = sum(
        1 for child in method_cursor.get_children()
        if child.kind == ci.CursorKind.PARM_DECL
    )
    for child in method_cursor.get_children():
        if child.kind == ci.CursorKind.PARM_DECL:
            p_resolved = resolve_type(child.type, child.spelling, class_name,
                                      diag=diag, source_file=source_file,
                                      line=child.location.line - _PREAMBLE_LINE_COUNT)
            if p_resolved is None:
                continue
            if len(p_resolved) >= 5:
                params.append(ParamInfo(
                    name=child.spelling,
                    cpp_type=_type_spelling_for_emission(child.type),
                    property_class=p_resolved[0],
                    is_vector=True,
                    inner_cpp_type=p_resolved[3],
                    inner_property_class=p_resolved[4],
                    inner_is_object_ptr=p_resolved[5],
                    inner_pointee_type=p_resolved[6],
                ))
            else:
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
        return_is_vector=ret_is_vec,
        return_inner_cpp_type=ret_inner_cpp,
        return_inner_property_class=ret_inner_prop,
        return_inner_is_object_ptr=ret_inner_obj,
        return_inner_pointee_type=ret_inner_pt,
    )


def _parse_class(tu, class_cursor, source_file, include_path, source: str, *,
                  is_struct=False, is_abstract=False, metadata=None, diag=None):
    class_name = class_cursor.spelling
    file_name = str(source_file)
    info = ClassInfo(
        name=class_name,
        source_file=source_file,
        include_path=include_path,
        is_struct=is_struct,
        is_abstract=is_abstract,
        metadata=metadata or {},
    )

    # Pre-collect DFUNCTION macro line numbers for scope-correct matching
    dfunction_lines = _collect_dfunction_lines(tu, class_cursor)
    consumed_dfunction_lines: set[int] = set()

    for child in class_cursor.get_children():
        if child.kind == ci.CursorKind.CXX_BASE_SPECIFIER and not info.base_name:
            base_name = _base_class_name_from_cursor(child)
            if base_name:
                info.base_name = base_name
            continue

        # Warn on non-default constructors
        if child.kind == ci.CursorKind.CONSTRUCTOR and diag:
            param_count = sum(
                1 for c in child.get_children()
                if c.kind == ci.CursorKind.PARM_DECL
            )
            if param_count > 0:
                src_line = child.location.line - _PREAMBLE_LINE_COUNT
                diag.warn(file_name, src_line,
                          f"DCLASS '{class_name}' has non-default constructor "
                          f"'{child.displayname}' — CreateDObject only calls "
                          f"the default constructor")

        if child.kind == ci.CursorKind.FIELD_DECL:
            if not _is_annotated(tu, child, "DPROPERTY"):
                continue
            resolved = resolve_type(child.type, child.spelling, class_name,
                                    diag=diag, source_file=file_name,
                                    line=child.location.line - _PREAMBLE_LINE_COUNT,
                                    tu=tu)
            if resolved is None:
                continue
            is_dstruct_field = len(resolved) == 4 and resolved[0] == "DStructProperty"
            if is_dstruct_field:
                prop_class = resolved[0]
                is_obj_ptr = False
                pointee = ""
                dstruct_type_name = resolved[3]
                is_vec = False
                inner_cpp = ""
                inner_prop = ""
                inner_is_obj_ptr = False
                inner_pointee = ""
            else:
                prop_class, is_obj_ptr, pointee = resolved[0], resolved[1], resolved[2]
                is_vec = len(resolved) >= 5
                inner_cpp = resolved[3] if is_vec else ""
                inner_prop = resolved[4] if is_vec else ""
                inner_is_obj_ptr = resolved[5] if len(resolved) >= 7 else False
                inner_pointee = resolved[6] if len(resolved) >= 7 else ""
                dstruct_type_name = ""
            offset_bits = class_cursor.type.get_offset(child.spelling)
            offset_bytes = offset_bits // 8 if offset_bits >= 0 else -1
            dprop_args = _extract_macro_args(tu, child, "DPROPERTY") or ""
            prop_metadata = _parse_dproperty_meta(dprop_args)
            editor_only = bool(re.search(r'\bEditorOnly\b', dprop_args))
            hide_in_details = bool(re.search(r'\bHideInDetails\b', dprop_args))
            info.properties.append(PropertyInfo(
                name=child.spelling,
                cpp_type=child.type.spelling,
                property_class=prop_class,
                offset=offset_bytes,
                is_object_ptr=is_obj_ptr,
                pointee_type=pointee,
                metadata=prop_metadata,
                is_vector=is_vec,
                inner_cpp_type=inner_cpp,
                inner_property_class=inner_prop,
                inner_is_object_ptr=inner_is_obj_ptr,
                inner_pointee_type=inner_pointee,
                is_dstruct=is_dstruct_field,
                dstruct_type_name=dstruct_type_name,
                editor_only=editor_only,
                hide_in_details=hide_in_details,
            ))

        elif child.kind == ci.CursorKind.FUNCTION_TEMPLATE:
            # Check if a DFUNCTION annotation precedes this template method
            method_line = child.location.line
            for dl, _dargs in dfunction_lines:
                if dl in consumed_dfunction_lines:
                    continue
                if dl < method_line:
                    consumed_dfunction_lines.add(dl)
                    if diag:
                        src_line = method_line - _PREAMBLE_LINE_COUNT
                        diag.warn(file_name, src_line,
                                  f"DFUNCTION() on template function "
                                  f"'{child.spelling}' is not supported "
                                  f"and will be ignored")
                    break

        elif child.kind == ci.CursorKind.CXX_METHOD:
            if is_struct:
                continue

            method_line = child.location.line
            matched = False
            matched_args = ""
            for dl, dargs in dfunction_lines:
                if dl in consumed_dfunction_lines:
                    continue
                if dl < method_line:
                    matched = True
                    matched_args = dargs
                    consumed_dfunction_lines.add(dl)
                    break

            if not matched:
                continue

            # Warn if the DFUNCTION is defined inline in the header
            if child.is_definition() and diag:
                src_line = method_line - _PREAMBLE_LINE_COUNT
                diag.warn(file_name, src_line,
                          f"DFUNCTION() '{child.spelling}' is inline "
                          f"— move definition to .cpp")

            fn = _parse_function(tu, child, class_name,
                                 diag=diag, source_file=file_name)
            if fn is None:
                continue
            fn.metadata = _parse_dfunction_meta(matched_args)
            if "ShowAsButton" in fn.metadata and len(fn.params) != 0 and diag:
                src_line = method_line - _PREAMBLE_LINE_COUNT
                diag.warn(file_name, src_line,
                          f"DFUNCTION(ShowAsButton) '{fn.name}' has "
                          f"{len(fn.params)} parameter(s); the details panel "
                          f"only renders a button for 0-parameter functions")
            same_name_count = sum(1 for f in info.functions if f.name == fn.name)
            fn.overload_index = same_name_count + 1
            info.functions.append(fn)

    # Fallback: text-scan for DPROPERTY fields missing from AST (e.g. std::string with stub types)
    existing_names = {p.name for p in info.properties}
    try:
        extent = class_cursor.extent
        start_line = extent.start.line if extent.start else class_cursor.location.line
        end_line = extent.end.line if extent.end else start_line + 500
        text_fields = _extract_dproperty_fields_from_source(
            source, class_name, start_line, end_line
        )
        for field_name, type_str, args_str in text_fields:
            if field_name in existing_names:
                continue
            resolved = resolve_type_from_string(type_str, field_name, class_name)
            if resolved is not None:
                prop_class, is_obj_ptr, pointee = resolved[0], resolved[1], resolved[2]
                is_vec = len(resolved) >= 5
                inner_cpp = resolved[3] if is_vec else ""
                inner_prop = resolved[4] if is_vec else ""
                inner_is_obj_ptr = resolved[5] if len(resolved) >= 7 else False
                inner_pointee = resolved[6] if len(resolved) >= 7 else ""
                editor_only = bool(re.search(r'\bEditorOnly\b', args_str or ""))
                hide_in_details = bool(re.search(r'\bHideInDetails\b', args_str or ""))
                info.properties.append(PropertyInfo(
                    name=field_name,
                    cpp_type=type_str,
                    property_class=prop_class,
                    offset=-1,
                    is_object_ptr=is_obj_ptr,
                    pointee_type=pointee,
                    is_vector=is_vec,
                    inner_cpp_type=inner_cpp,
                    inner_property_class=inner_prop,
                    inner_is_object_ptr=inner_is_obj_ptr,
                    inner_pointee_type=inner_pointee,
                    editor_only=editor_only,
                    hide_in_details=hide_in_details,
                ))
                existing_names.add(field_name)
    except Exception:
        pass

    # Fallback: libclang often omits CXX_BASE_SPECIFIER with stripped/incomplete parse
    if not info.base_name and class_name != "DObject" and source:
        info.base_name = _extract_base_from_source(
            source, class_name, class_cursor.location.line
        )

    return info


# ── public API ───────────────────────────────────────────────


def parse_header(
    file_path: Path,
    input_dir: Path,
    extra_include_dirs: list[Path] | None = None,
) -> ParseResult:
    _ensure_configured()

    diag = DiagnosticCollector()

    file_path = file_path.resolve()
    input_dir = input_dir.resolve()
    engine_include_root = input_dir.parent  # e.g. Engine/

    raw = file_path.read_text(encoding="utf-8", errors="replace")
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
    for cursor in tu.cursor.walk_preorder():
        if cursor.location.file is None:
            continue
        if str(Path(cursor.location.file.name).resolve()) != file_str:
            continue
        if cursor.kind not in (ci.CursorKind.CLASS_DECL, ci.CursorKind.STRUCT_DECL):
            continue
        if not cursor.is_definition():
            continue
        if _is_anonymous_or_invalid(cursor.spelling or ""):
            continue

        is_dclass = _is_annotated(tu, cursor, "DCLASS")
        is_dstruct = _is_annotated(tu, cursor, "DSTRUCT")

        if not is_dclass and not is_dstruct:
            continue

        is_abstract = False
        macro_args = ""
        if is_dclass:
            macro_args = _extract_macro_args(tu, cursor, "DCLASS") or ""
            if macro_args and "abstract" in macro_args:
                is_abstract = True
            if not is_abstract:
                is_abstract = cursor.is_abstract_record()
        elif is_dstruct:
            macro_args = _extract_macro_args(tu, cursor, "DSTRUCT") or ""

        class_metadata = _parse_meta_kv(macro_args)

        results.append(_parse_class(
            tu, cursor, file_path, include_path, stripped_source,
            is_struct=is_dstruct,
            is_abstract=is_abstract,
            metadata=class_metadata,
            diag=diag,
        ))

    return ParseResult(
        classes=results,
        source_includes=source_includes,
        forward_decls=forward_decls,
        diagnostics=diag,
    )
