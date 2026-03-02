import re
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
    "std::wstring":                       "DWStringProperty",
    "std::basic_string<wchar_t>":         "DWStringProperty",
    "DirectX::SimpleMath::Vector3":       "DVector3Property",
    "DirectX::SimpleMath::Quaternion":    "DQuaternionProperty",
    "DirectX::XMMATRIX":                  "DFloat4x4Property",
    "DirectX::XMVECTOR":                  "DFloat4Property",
}

_SHARED_PTR_RE = re.compile(r"^std::shared_ptr<(.+)>$")


def normalize_type_spelling(spelling: str) -> str:
    """Normalize verbose STL spellings to stable aliases used by codegen."""
    s = _strip_const(spelling.strip())

    # Fast-path canonical libclang spellings for std::string/std::wstring.
    s = re.sub(
        r"^std::basic_string<\s*char\s*,\s*std::char_traits<char>\s*,\s*std::allocator<char>\s*>$",
        "std::string",
        s,
    )
    s = re.sub(
        r"^std::basic_string<\s*wchar_t\s*,\s*std::char_traits<wchar_t>\s*,\s*std::allocator<wchar_t>\s*>$",
        "std::wstring",
        s,
    )

    # Generic basic_* alias collapsing when all template args are defaults.
    # Example handled: std::basic_string<char, std::char_traits<char>, std::allocator<char>>
    m = re.match(r"^std::basic_([a-zA-Z_][a-zA-Z0-9_]*)<(.+)>$", s)
    if not m:
        return s

    basic_name = m.group(1)
    args = _split_template_args(m.group(2))
    if basic_name == "string" and len(args) >= 2:
        char_t = args[0].strip()
        traits_t = args[1].strip()
        alloc_t = args[2].strip() if len(args) >= 3 else ""

        if traits_t == f"std::char_traits<{char_t}>":
            if not alloc_t or alloc_t == f"std::allocator<{char_t}>":
                alias_by_char = {
                    "char": "std::string",
                    "wchar_t": "std::wstring",
                    "char8_t": "std::u8string",
                    "char16_t": "std::u16string",
                    "char32_t": "std::u32string",
                }
                return alias_by_char.get(char_t, s)

    return s


def resolve_type(cursor_type, field_name="", class_name=""):
    """Resolve a clang Type to (property_class, is_object_ptr, pointee_type).

    Returns None if the type is unrecognized.
    """
    if cursor_type.kind == TypeKind.LVALUEREFERENCE:
        return resolve_type(cursor_type.get_pointee(), field_name, class_name)

    if cursor_type.kind == TypeKind.POINTER:
        pointee = cursor_type.get_pointee()
        pointee_name = _strip_namespaces(pointee.spelling)
        return (f"DObjectPtrProperty<{pointee_name}>", True, pointee_name)

    spelling = normalize_type_spelling(cursor_type.spelling)
    resolved = resolve_type_spelling(spelling, field_name, class_name)
    if resolved is not None:
        return resolved

    canonical = normalize_type_spelling(cursor_type.get_canonical().spelling)
    return resolve_type_spelling(canonical, field_name, class_name)


def resolve_type_spelling(spelling: str, field_name="", class_name=""):
    s = normalize_type_spelling(spelling)
    m = _SHARED_PTR_RE.match(s)
    if m:
        inner = _strip_namespaces(m.group(1))
        return (f"DSharedObjectPtrProperty<{inner}>", True, inner)

    prop = TYPE_MAP.get(s)
    if prop:
        return (prop, False, "")

    return None


def _strip_const(spelling: str) -> str:
    """Remove leading 'const ' qualifier from a type spelling."""
    if spelling.startswith("const "):
        return spelling[6:]
    return spelling


def _split_template_args(args_text: str) -> list[str]:
    """Split template args while respecting nested <...>."""
    args: list[str] = []
    current: list[str] = []
    depth = 0
    for ch in args_text:
        if ch == "<":
            depth += 1
            current.append(ch)
        elif ch == ">":
            depth -= 1
            current.append(ch)
        elif ch == "," and depth == 0:
            args.append("".join(current).strip())
            current = []
        else:
            current.append(ch)
    if current:
        args.append("".join(current).strip())
    return args


def _strip_namespaces(name):
    """Strip all namespace qualifiers: 'A::B::C' -> 'C'."""
    return name.rsplit("::", 1)[-1] if "::" in name else name
