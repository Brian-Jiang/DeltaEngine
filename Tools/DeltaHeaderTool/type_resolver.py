import sys
from clang.cindex import TypeKind


TYPE_MAP = {
    "int":                                "DIntProperty",
    "int32_t":                            "DIntProperty",
    "float":                              "DFloatProperty",
    "bool":                               "DBoolProperty",
    "double":                             "DDoubleProperty",
    "std::string":                        "DStringProperty",
    "std::basic_string<char>":            "DStringProperty",
    "DirectX::SimpleMath::Vector3":       "DVector3Property",
    "DirectX::SimpleMath::Quaternion":    "DQuaternionProperty",
}


def resolve_type(cursor_type, field_name="", class_name=""):
    """Resolve a clang Type to (property_class, is_object_ptr, pointee_type).

    Returns None if the type is unrecognized.
    """
    if cursor_type.kind == TypeKind.POINTER:
        pointee = cursor_type.get_pointee()
        pointee_name = _strip_namespaces(pointee.spelling)
        return (f"DObjectPtrProperty<{pointee_name}>", True, pointee_name)

    spelling = cursor_type.spelling
    prop = TYPE_MAP.get(spelling)
    if prop:
        return (prop, False, "")

    canonical = cursor_type.get_canonical().spelling
    prop = TYPE_MAP.get(canonical)
    if prop:
        return (prop, False, "")

    print(
        f"WARNING: unknown type '{spelling}' for property "
        f"'{field_name}' in '{class_name}' — skipping",
        file=sys.stderr,
    )
    return None


def _strip_namespaces(name):
    """Strip all namespace qualifiers: 'A::B::C' -> 'C'."""
    return name.rsplit("::", 1)[-1] if "::" in name else name
