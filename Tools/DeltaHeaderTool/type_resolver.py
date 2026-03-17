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

_SHARED_PTR_RE = re.compile(r"^std::shared_ptr<(.+)>$")
_STRING_RE = re.compile(r"^std::(?:string|basic_string\s*<\s*char\b)")
_WSTRING_RE = re.compile(r"^std::(?:wstring|basic_string\s*<\s*wchar_t\b)")
_VECTOR_RE = re.compile(r"^std::vector\s*<")


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
        if diag:
            diag.warn(source_file, line,
                      f"std::vector is not supported for reflection on property '{field_name}'; "
                      "use GetX/SetX accessors instead.")
        else:
            print(
                f"WARNING: std::vector is not supported for reflection on property "
                f"'{field_name}' in '{class_name}' — skipping",
                file=sys.stderr,
            )
        return None

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
        if diag:
            diag.warn(source_file, line,
                      f"std::vector is not supported for reflection on property '{field_name}'; "
                      "use GetX/SetX accessors instead.")
        else:
            print(
                f"WARNING: std::vector is not supported for reflection on property "
                f"'{field_name}' in '{class_name}' — skipping",
                file=sys.stderr,
            )
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


def resolve_type_from_string(type_str: str, field_name: str = "", class_name: str = "") -> tuple[str, bool, str] | None:
    """Resolve a type string (from source text) to (property_class, is_object_ptr, pointee_type).
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
        return None

    return None


def _strip_const(spelling: str) -> str:
    """Remove leading 'const ' qualifier from a type spelling."""
    if spelling.startswith("const "):
        return spelling[6:]
    return spelling


def _strip_namespaces(name):
    """Strip all namespace qualifiers: 'A::B::C' -> 'C'."""
    return name.rsplit("::", 1)[-1] if "::" in name else name
