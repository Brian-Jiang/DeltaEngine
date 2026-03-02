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
}

_SHARED_PTR_RE = re.compile(r"^std::shared_ptr<(.+)>$")


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

    spelling = _strip_const(cursor_type.spelling)

    m = _SHARED_PTR_RE.match(spelling)
    if m:
        inner = _strip_namespaces(m.group(1))
        return (f"DSharedObjectPtrProperty<{inner}>", True, inner)

    prop = TYPE_MAP.get(spelling)
    if prop:
        return (prop, False, "")

    canonical = _strip_const(cursor_type.get_canonical().spelling)
    prop = TYPE_MAP.get(canonical)
    if prop:
        return (prop, False, "")

    m = _SHARED_PTR_RE.match(canonical)
    if m:
        inner = _strip_namespaces(m.group(1))
        return (f"DSharedObjectPtrProperty<{inner}>", True, inner)

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


def _strip_const(spelling: str) -> str:
    """Remove leading 'const ' qualifier from a type spelling."""
    if spelling.startswith("const "):
        return spelling[6:]
    return spelling


def _strip_namespaces(name):
    """Strip all namespace qualifiers: 'A::B::C' -> 'C'."""
    return name.rsplit("::", 1)[-1] if "::" in name else name
