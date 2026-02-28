from __future__ import annotations

import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

import clang.cindex as ci

from type_resolver import resolve_type

TOOL_DIR = Path(__file__).resolve().parent
TOOLS_DIR = TOOL_DIR.parent
LIB_PATH = TOOLS_DIR / "Clang"

_configured = False

# Strip #include lines so clang never touches the filesystem for dependencies.
_INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"].*?[>"]', re.MULTILINE)

# Capture full #include directives for re-emission (exact line including quote/angle).
_INCLUDE_CAPTURE_RE = re.compile(r'^\s*#\s*include\s+[<"][^">]+[">]', re.MULTILINE)

# Preamble: stubs and forward declarations so stripped headers parse well enough
# for DCLASS/DPROPERTY/DFUNCTION extraction. We tolerate unresolved-type errors.
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
struct XMMATRIX { float m[4][4]; };
struct __declspec(align(16)) XMVECTOR { float f[4]; };
typedef XMVECTOR XMFLOAT2;
typedef XMVECTOR XMFLOAT3;
typedef XMVECTOR XMFLOAT4;
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
#define DPROPERTY(...)
#define DFUNCTION(...)
#define DGENERATED_BODY(ClassName)

#define DELTA_ENGINE_NS_BEGIN  namespace DeltaEngine {
#define DELTA_ENGINE_NS_END    }
#define DELTAENGINE_API
"""

# Line count of the preamble; declarations at line <= this are from the preamble, not the source file.
_PREAMBLE_LINE_COUNT = len(_PREAMBLE.splitlines())


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
    base_name: str = ""
    properties: list[PropertyInfo] = field(default_factory=list)
    functions: list[FunctionInfo] = field(default_factory=list)


@dataclass
class ParseResult:
    """Result of parsing one header: reflected classes plus file-level data."""
    classes: list[ClassInfo]
    source_includes: list[str]
    forward_decls: list[tuple[str, str]]  # (kind, name) with kind in ("class", "struct", "template_class")


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
    if spelling.strip().startswith("(anonymous)"):
        return True
    return False


def _collect_forward_decls(tu, file_str: str, source_line_start: int) -> list[tuple[str, str]]:
    """Walk AST and collect (kind, name) for every class/struct/class_template declared in the source file.
    Includes both definitions and forward declarations (e.g. 'class GameObject;').
    Cursors at line <= source_line_start are from the preamble and are excluded."""
    decls: list[tuple[str, str]] = []
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
        decls.append((kind, spelling))
    return list(dict.fromkeys(decls))


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
        if child.kind == ci.CursorKind.CXX_BASE_SPECIFIER and not info.base_name:
            info.base_name = child.spelling
            continue

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
) -> ParseResult:
    _ensure_configured()

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
        if not _is_annotated(tu, cursor, "DCLASS"):
            continue

        results.append(_parse_class(tu, cursor, file_path, include_path))

    return ParseResult(
        classes=results,
        source_includes=source_includes,
        forward_decls=forward_decls,
    )
