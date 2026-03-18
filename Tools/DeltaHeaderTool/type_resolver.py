import re
import sys
from clang.cindex import TypeKind


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
    "DirectX::XMVECTOR":                  "DFloat4Property",
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

_SHARED_PTR_RE = re.compile(r"^std::shared_ptr<(.+)>$")
_STRING_RE = re.compile(r"^std::(?:string|basic_string\s*<\s*char\b)")
_WSTRING_RE = re.compile(r"^std::(?:wstring|basic_string\s*<\s*wchar_t\b)")
_VECTOR_RE = re.compile(r"^std::vector\s*<")


def _extract_vector_inner_type(spelling: str) -> str | None:
    """Extract the first template argument from std::vector<T, ...>.
    Uses bracket-aware parsing to handle nested templates correctly."""
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


def _resolve_vector_inner(inner_type: str) -> tuple[str, str] | None:
    """Resolve a vector inner type string to (inner_property_class, inner_cpp_type).
    Returns None if the inner type is not supported as a vector element."""
    s = _strip_elaborated(_strip_const(inner_type.strip()))

    prop = TYPE_MAP.get(s)
    if prop and prop in VECTOR_ELEMENT_PROPERTY_CLASSES:
        return (prop, INNER_TYPE_TO_CPP.get(prop, s))

    if _STRING_RE.match(s):
        return ("DStringProperty", "std::string")
    if _WSTRING_RE.match(s):
        return ("DWStringProperty", "std::wstring")

    return None


def _try_resolve_vector(spelling, field_name, class_name, *, diag=None, source_file="", line=0):
    """Try to resolve a std::vector<T> type. Returns (property_class, False, "", inner_cpp, inner_prop) or None."""
    inner_type = _extract_vector_inner_type(spelling)
    if inner_type is None:
        return None

    resolved = _resolve_vector_inner(inner_type)
    if resolved is None:
        msg = (f"std::vector<{inner_type}> on property '{field_name}' — "
               f"inner type '{inner_type}' has no supported DProperty subclass. "
               f"Only value types (float, int, bool, double, string, Vector3, etc.) are supported.")
        if diag:
            diag.warn(source_file, line, msg)
        else:
            print(f"WARNING: {msg} in '{class_name}' — skipping", file=sys.stderr)
        return None

    inner_prop_class, inner_cpp = resolved
    return ("DVectorProperty", False, "", inner_cpp, inner_prop_class)


def _strip_elaborated(spelling: str) -> str:
    """Remove leading 'class ' and 'struct ' qualifiers from type spelling."""
    s = spelling.strip()
    for prefix in ("class ", "struct "):
        if s.startswith(prefix):
            return s[len(prefix):].strip()
    return s


def resolve_type(cursor_type, field_name="", class_name="", *,
                  diag=None, source_file="", line=0):
    """Resolve a clang Type to (property_class, is_object_ptr, pointee_type).

    For vector types returns (property_class, False, "", inner_cpp_type, inner_property_class).
    Returns None if the type is unrecognized.
    """
    if cursor_type.kind == TypeKind.LVALUEREFERENCE:
        return resolve_type(cursor_type.get_pointee(), field_name, class_name,
                            diag=diag, source_file=source_file, line=line)

    if cursor_type.kind == TypeKind.POINTER:
        pointee = cursor_type.get_pointee()
        pointee_name = _strip_namespaces(pointee.spelling)
        return (f"DObjectPtrProperty<{pointee_name}>", True, pointee_name)

    spelling = _strip_elaborated(_strip_const(cursor_type.spelling))

    m = _SHARED_PTR_RE.match(spelling)
    if m:
        inner = _strip_namespaces(m.group(1))
        return (f"DSharedObjectPtrProperty<{inner}>", True, inner)

    prop = TYPE_MAP.get(spelling)
    if prop:
        return (prop, False, "")

    if _STRING_RE.match(spelling):
        return ("DStringProperty", False, "")
    if _WSTRING_RE.match(spelling):
        return ("DWStringProperty", False, "")

    if _VECTOR_RE.match(spelling):
        return _try_resolve_vector(spelling, field_name, class_name,
                                   diag=diag, source_file=source_file, line=line)

    canonical = _strip_elaborated(_strip_const(cursor_type.get_canonical().spelling))
    prop = TYPE_MAP.get(canonical)
    if prop:
        return (prop, False, "")

    m = _SHARED_PTR_RE.match(canonical)
    if m:
        inner = _strip_namespaces(m.group(1))
        return (f"DSharedObjectPtrProperty<{inner}>", True, inner)

    if _STRING_RE.match(canonical):
        return ("DStringProperty", False, "")
    if _WSTRING_RE.match(canonical):
        return ("DWStringProperty", False, "")

    if _VECTOR_RE.match(canonical):
        return _try_resolve_vector(canonical, field_name, class_name,
                                   diag=diag, source_file=source_file, line=line)

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
    """Resolve a type string (from source text) to (property_class, is_object_ptr, pointee_type).
    For vectors returns 5-tuple: (property_class, False, "", inner_cpp, inner_prop_class).
    Used as fallback when AST misses fields (e.g. std::string with stub types)."""
    s = _strip_const(type_str.strip())
    for prefix in ("class ", "struct "):
        if s.startswith(prefix):
            s = s[len(prefix):].strip()
            break

    if s.endswith("*"):
        pointee = s[:-1].strip()
        pointee_name = _strip_namespaces(pointee)
        return (f"DObjectPtrProperty<{pointee_name}>", True, pointee_name)

    m = _SHARED_PTR_RE.match(s)
    if m:
        inner = _strip_namespaces(m.group(1))
        return (f"DSharedObjectPtrProperty<{inner}>", True, inner)

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
    """Remove leading 'const ' qualifier from a type spelling."""
    if spelling.startswith("const "):
        return spelling[6:]
    return spelling


def _strip_namespaces(name):
    """Strip all namespace qualifiers: 'A::B::C' -> 'C'."""
    return name.rsplit("::", 1)[-1] if "::" in name else name
