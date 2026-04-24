import re
import sys
from clang.cindex import CursorKind, TypeKind

from macro_utils import is_annotated


TYPE_MAP = {
    "int":                                "DIntProperty",
    "int32_t":                            "DIntProperty",
    "unsigned int":                       "DIntProperty",
    "float":                              "DFloatProperty",
    "bool":                               "DBoolProperty",
    "double":                             "DDoubleProperty",
    "std::string":                        "DStringProperty",
    "std::basic_string<char>":            "DStringProperty",
    "std::basic_string<char, std::char_traits<char>>": "DStringProperty",
    "std::basic_string<char, std::char_traits<char>, std::allocator<char>>": "DStringProperty",
    "std::wstring":                       "DWStringProperty",
    "std::basic_string<wchar_t>":         "DWStringProperty",
    "std::basic_string<wchar_t, std::char_traits<wchar_t>>": "DWStringProperty",
    "std::basic_string<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>>": "DWStringProperty",
    "DirectX::SimpleMath::Vector3":       "DVector3Property",
    "DirectX::SimpleMath::Quaternion":    "DQuaternionProperty",
    "DirectX::XMMATRIX":                  "DFloat4x4Property",
    "DirectX::XMFLOAT4X4":                "DFloat4x4Property",
    "DirectX::XMVECTOR":                  "DFloat4Property",
    "DirectX::XMFLOAT4":                  "DFloat4Property",
    "TBulkData":                          "DBulkDataProperty",
    "DeltaEngine::TBulkData":             "DBulkDataProperty",
}

VECTOR_ELEMENT_PROPERTY_CLASSES = {
    "DFloatProperty", "DIntProperty", "DBoolProperty", "DDoubleProperty",
    "DStringProperty", "DWStringProperty",
    "DVector3Property", "DQuaternionProperty",
    "DFloat4Property", "DFloat4x4Property",
}

INNER_TYPE_TO_CPP = {
    "DFloatProperty":      "float",
    "DIntProperty":        "int",
    "DBoolProperty":       "bool",
    "DDoubleProperty":     "double",
    "DStringProperty":     "std::string",
    "DWStringProperty":    "std::wstring",
    "DVector3Property":    "DirectX::SimpleMath::Vector3",
    "DQuaternionProperty": "DirectX::SimpleMath::Quaternion",
    "DFloat4Property":     "DirectX::XMFLOAT4",
    "DFloat4x4Property":   "DirectX::XMFLOAT4X4",
}

_STRING_RE = re.compile(r"^std::(?:string|basic_string\s*<\s*char\b)")
_WSTRING_RE = re.compile(r"^std::(?:wstring|basic_string\s*<\s*wchar_t\b)")
_VECTOR_RE = re.compile(r"^std::vector\s*<")


def _extract_vector_inner_type(spelling: str) -> str | None:
    m = re.match(r'^(?:std::)?vector\s*<\s*', spelling)
    if not m:
        return None
    start = m.end()
    depth = 0
    for i in range(start, len(spelling)):
        ch = spelling[i]
        if ch == '<':
            depth += 1
        elif ch == '>':
            if depth == 0:
                return spelling[start:i].strip()
            depth -= 1
        elif ch == ',' and depth == 0:
            return spelling[start:i].strip()
    return None


def _is_anonymous_name(name: str) -> bool:
    if not name or not name.strip():
        return True
    s = name.strip()
    return s.startswith("(anonymous") or s.startswith("(unnamed")


def _record_derives_from_dobject(decl_cursor):
    import clang.cindex as ci
    for child in decl_cursor.get_children():
        if child.kind != ci.CursorKind.CXX_BASE_SPECIFIER:
            continue
        t = child.type
        if t.kind == TypeKind.INVALID:
            continue
        sp = _strip_elaborated(_strip_const(t.spelling))
        base_name = _strip_namespaces(sp)
        if base_name == "DObject":
            return True
        base_decl = t.get_declaration()
        if base_decl and base_decl.kind in (ci.CursorKind.CLASS_DECL, ci.CursorKind.STRUCT_DECL):
            if _record_derives_from_dobject(base_decl):
                return True
    return False


def _resolve_vector_inner(inner_type: str) -> tuple | None:
    s = _strip_elaborated(_strip_const(inner_type.strip()))

    prop = TYPE_MAP.get(s)
    if prop and prop in VECTOR_ELEMENT_PROPERTY_CLASSES:
        return (prop, INNER_TYPE_TO_CPP.get(prop, s), False, "")

    if _STRING_RE.match(s):
        return ("DStringProperty", "std::string", False, "")
    if _WSTRING_RE.match(s):
        return ("DWStringProperty", "std::wstring", False, "")

    if s.endswith("*"):
        pointee_raw = s[:-1].strip()
        pointee = _strip_namespaces(_strip_elaborated(pointee_raw))
        return (f"DObjectPtrProperty<{pointee}>", s, True, pointee)

    if _VECTOR_RE.match(s):
        nested_inner = _extract_vector_inner_type(s)
        if nested_inner is not None:
            nested = _resolve_vector_inner(nested_inner)
            if nested is not None:
                nested_inner_prop, nested_inner_cpp, _, _ = nested
                return ("DVectorProperty", f"std::vector<{nested_inner_cpp}>", False, "")

    return None


def _resolve_vector_inner_from_type(tu, inner_t, field_name, class_name, diag, source_file, line):
    import clang.cindex as ci
    if inner_t.kind == TypeKind.POINTER:
        pointee = inner_t.get_pointee()
        decl = pointee.get_declaration()
        if decl.kind in (ci.CursorKind.CLASS_DECL, ci.CursorKind.STRUCT_DECL):
            if is_annotated(tu, decl, "DSTRUCT"):
                msg = (f"std::vector<{inner_t.spelling}> — DSTRUCT types cannot be used as pointers "
                       f"in DPROPERTY; use value type or remove DPROPERTY.")
                if diag:
                    diag.warn(source_file, line, msg)
                return None
        return _resolve_vector_inner(inner_t.spelling)

    decl = inner_t.get_declaration()
    if decl.kind in (ci.CursorKind.CLASS_DECL, ci.CursorKind.STRUCT_DECL):
        if is_annotated(tu, decl, "DSTRUCT"):
            name = decl.spelling or ""
            if _is_anonymous_name(name):
                return None
            return ("DStructProperty", name, False, name)  # inner prop, cpp, obj_ptr, pt

    return _resolve_vector_inner(inner_t.spelling)


def _try_resolve_vector(spelling, field_name, class_name, *, diag=None, source_file="", line=0,
                        tu=None, cursor_type=None):
    inner_type = _extract_vector_inner_type(spelling)
    if inner_type is None:
        return None

    resolved = None
    if tu is not None and cursor_type is not None:
        try:
            n = cursor_type.get_num_template_arguments()
            if n > 0:
                inner_t = cursor_type.get_template_argument_type(0)
                resolved = _resolve_vector_inner_from_type(
                    tu, inner_t, field_name, class_name, diag, source_file, line)
        except Exception:
            resolved = None

    if resolved is None:
        resolved = _resolve_vector_inner(inner_type)
    if resolved is None:
        msg = (f"std::vector<{inner_type}> on property '{field_name}' — "
               f"inner type '{inner_type}' has no supported DProperty subclass. "
               f"Supported: value types, T*, std::vector<T>, DSTRUCT.")
        if diag:
            diag.warn(source_file, line, msg)
        else:
            print(f"WARNING: {msg} in '{class_name}' — skipping", file=sys.stderr)
        return None

    inner_prop_class, inner_cpp, is_inner_obj_ptr, inner_pointee = resolved
    return ("DVectorProperty", False, "", inner_cpp, inner_prop_class, is_inner_obj_ptr, inner_pointee)


def _strip_elaborated(spelling: str) -> str:
    s = spelling.strip()
    for prefix in ("class ", "struct "):
        if s.startswith(prefix):
            return s[len(prefix):].strip()
    return s


def resolve_type(cursor_type, field_name="", class_name="", *,
                  diag=None, source_file="", line=0, tu=None):
    """Resolve a clang Type to (property_class, is_object_ptr, pointee_type).

    For DStruct value fields: 4-tuple (DStructProperty, False, "", struct_name).
    For vector: 7-tuple with inner types.
    """
    import clang.cindex as ci
    if cursor_type.kind == TypeKind.LVALUEREFERENCE:
        return resolve_type(cursor_type.get_pointee(), field_name, class_name,
                            diag=diag, source_file=source_file, line=line, tu=tu)

    if cursor_type.kind == TypeKind.POINTER:
        pointee = cursor_type.get_pointee()
        decl = pointee.get_declaration()
        if tu is not None and decl.kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL):
            if is_annotated(tu, decl, "DSTRUCT"):
                msg = (f"DPROPERTY '{field_name}' cannot use pointer to DSTRUCT type "
                       f"'{decl.spelling}' — use value type.")
                if diag:
                    diag.warn(source_file, line, msg)
                else:
                    print(f"WARNING: {msg}", file=sys.stderr)
                return None
        pointee_name = _strip_namespaces(pointee.spelling)
        return (f"DObjectPtrProperty<{pointee_name}>", True, pointee_name)

    spelling = _strip_elaborated(_strip_const(cursor_type.spelling))

    prop = TYPE_MAP.get(spelling)
    if prop:
        return (prop, False, "")

    if _STRING_RE.match(spelling):
        return ("DStringProperty", False, "")
    if _WSTRING_RE.match(spelling):
        return ("DWStringProperty", False, "")

    if _VECTOR_RE.match(spelling):
        return _try_resolve_vector(spelling, field_name, class_name,
                                   diag=diag, source_file=source_file, line=line,
                                   tu=tu, cursor_type=cursor_type)

    canonical = _strip_elaborated(_strip_const(cursor_type.get_canonical().spelling))
    prop = TYPE_MAP.get(canonical)
    if prop:
        return (prop, False, "")

    if _STRING_RE.match(canonical):
        return ("DStringProperty", False, "")
    if _WSTRING_RE.match(canonical):
        return ("DWStringProperty", False, "")

    if _VECTOR_RE.match(canonical):
        return _try_resolve_vector(canonical, field_name, class_name,
                                   diag=diag, source_file=source_file, line=line,
                                   tu=tu, cursor_type=cursor_type)

    decl = cursor_type.get_declaration()
    if decl.kind in (CursorKind.CLASS_DECL, CursorKind.STRUCT_DECL) and tu is not None:
        name = decl.spelling or ""
        if _is_anonymous_name(name):
            if diag:
                diag.warn(source_file, line,
                          f"Cannot reflect anonymous type on property '{field_name}'")
            return None
        if is_annotated(tu, decl, "DSTRUCT"):
            return ("DStructProperty", False, "", name)
        if _record_derives_from_dobject(decl):
            msg = (f"DPROPERTY '{field_name}' cannot use DObject-derived type '{name}' "
                   f"by value — use a pointer.")
            if diag:
                diag.warn(source_file, line, msg)
            else:
                print(f"WARNING: {msg}", file=sys.stderr)
            return None

    if diag:
        diag.warn(source_file, line,
                  f"No corresponding property type found for '{spelling}' "
                  f"on property '{field_name}'")
    else:
        print(
            f"WARNING: unknown type '{spelling}' for property "
            f"'{field_name}' in '{class_name}' — skipping",
            file=sys.stderr,
        )
    return None


def resolve_type_from_string(type_str: str, field_name: str = "", class_name: str = "") -> tuple | None:
    s = _strip_const(type_str.strip())
    for prefix in ("class ", "struct "):
        if s.startswith(prefix):
            s = s[len(prefix):].strip()
            break

    if s.endswith("*"):
        pointee = s[:-1].strip()
        pointee_name = _strip_namespaces(pointee)
        return (f"DObjectPtrProperty<{pointee_name}>", True, pointee_name)

    prop = TYPE_MAP.get(s)
    if prop:
        return (prop, False, "")

    if _STRING_RE.match(s):
        return ("DStringProperty", False, "")
    if _WSTRING_RE.match(s):
        return ("DWStringProperty", False, "")

    if _VECTOR_RE.match(s):
        return _try_resolve_vector(s, field_name, class_name)

    return None


def _strip_const(spelling: str) -> str:
    if spelling.startswith("const "):
        return spelling[6:]
    return spelling


def _strip_namespaces(name):
    return name.rsplit("::", 1)[-1] if "::" in name else name
