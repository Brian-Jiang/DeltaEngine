from __future__ import annotations

import re

from parser import ClassInfo, FunctionInfo, ForwardDeclInfo
from templates import (
    FILE_HEADER,
    FILE_FOOTER_CLASS,
    FILE_FOOTER_NO_CREATE,
    THUNK_VOID_NO_PARAMS,
    THUNK_NO_PARAMS_WITH_RETURN,
    THUNK_WITH_PARAMS_NO_RETURN,
    THUNK_WITH_PARAMS_WITH_RETURN,
    THUNK_PARAM_EXTRACTION,
    DCLASS_REGISTRATION_BEGIN,
    DCLASS_REGISTRATION_BEGIN_ABSTRACT,
    DCLASS_REGISTRATION_END,
    DSTRUCT_REGISTRATION_BEGIN,
    DSTRUCT_REGISTRATION_END,
    DPROPERTY,
    DPROPERTY_WITH_META,
    DPROPERTY_OBJECT_PTR,
    DPROPERTY_VECTOR,
    DPROPERTY_VECTOR_OBJECT_PTR,
    DPROPERTY_DSTRUCT,
    DPROPERTY_VECTOR_DSTRUCT,
    DFUNCTION_VOID_NO_PARAMS,
    DFUNCTION_WITH_PARAMS,
    DFUNCTION_PARAM,
    DFUNCTION_PARAM_OBJECT_PTR,
    DFUNCTION_PARAM_VECTOR,
    DFUNCTION_PARAM_VECTOR_OBJECT_PTR,
    DFUNCTION_RETURN,
    DFUNCTION_RETURN_OBJECT_PTR,
    DFUNCTION_RETURN_VECTOR,
    DFUNCTION_RETURN_VECTOR_OBJECT_PTR,
    GENERATED_HEADER_FILE,
    GENERATED_HEADER_PARAMS_STRUCT,
    GENERATED_HEADER_PARAMS_FIELD,
    GENERATED_HEADER_CLASS_BLOCK,
    GENERATED_HEADER_CREATE_OBJECT,
)

_OBJECT_PTR_PROP_RE = re.compile(r"^DObjectPtrProperty<(.+)>$")

EXTRA_PROPERTY_HEADERS = {
    "DBulkDataProperty": "Runtime/Reflection/DBulkDataProperty.h",
    "DVectorProperty": "Runtime/Reflection/DVectorProperty.h",
}


def _format_meta_init(metadata: dict) -> str:
    """Format a Python dict as a C++ unordered_map initializer list."""
    pairs = ", ".join(f'{{"{k}", "{v}"}}' for k, v in metadata.items())
    return "{" + pairs + "}"


def _strip_namespaces(name: str) -> str:
    """Strip all namespace qualifiers: 'A::B::C' -> 'C'."""
    return name.rsplit("::", 1)[-1] if "::" in name else name


def _collect_cpp_full_includes(
    classes: list[ClassInfo],
    header_stem: str,
    type_to_header: dict[str, str],
) -> set[str]:
    """Collect header paths for DObject-derived types used in ObjectPtr."""
    cpp_full_includes: set[str] = set()
    source_include = f"{classes[0].include_path}{header_stem}.h"

    for cls in classes:
        for prop in cls.properties:
            if prop.is_object_ptr and prop.pointee_type:
                pointee = _strip_namespaces(prop.pointee_type)
                path = type_to_header.get(pointee)
                if path and path != source_include:
                    cpp_full_includes.add(path)
            # Include headers for object-pointer inner types of vector properties
            if prop.is_vector and prop.inner_is_object_ptr and prop.inner_pointee_type:
                pointee = _strip_namespaces(prop.inner_pointee_type)
                path = type_to_header.get(pointee)
                if path and path != source_include:
                    cpp_full_includes.add(path)

        for fn in cls.functions:
            for p in fn.params:
                m = _OBJECT_PTR_PROP_RE.match(p.property_class)
                if m:
                    pointee = _strip_namespaces(m.group(1))
                    path = type_to_header.get(pointee)
                    if path and path != source_include:
                        cpp_full_includes.add(path)
            if fn.return_property_class:
                m = _OBJECT_PTR_PROP_RE.match(fn.return_property_class)
                if m:
                    pointee = _strip_namespaces(m.group(1))
                    path = type_to_header.get(pointee)
                    if path and path != source_include:
                        cpp_full_includes.add(path)
            dv = EXTRA_PROPERTY_HEADERS["DVectorProperty"]
            for p in fn.params:
                if p.is_vector:
                    cpp_full_includes.add(dv)
                    if p.inner_is_object_ptr and p.inner_pointee_type:
                        pt = _strip_namespaces(p.inner_pointee_type)
                        path = type_to_header.get(pt)
                        if path and path != source_include:
                            cpp_full_includes.add(path)
            if fn.return_is_vector:
                cpp_full_includes.add(dv)
                if fn.return_inner_is_object_ptr and fn.return_inner_pointee_type:
                    pt = _strip_namespaces(fn.return_inner_pointee_type)
                    path = type_to_header.get(pt)
                    if path and path != source_include:
                        cpp_full_includes.add(path)

    return cpp_full_includes


# ── header generation ────────────────────────────────────────


def _overload_suffix(overload_index: int) -> str:
    """Suffix for params struct and thunk when overload_index > 1: _2, _3, ..."""
    return "" if overload_index <= 1 else f"_{overload_index}"


def _params_structs_for_class(cls: ClassInfo) -> str:
    """Build params struct block for one class."""
    parts = ""
    for fn in cls.functions:
        if not fn.has_params_struct:
            continue
        fields = ""
        for p in fn.params:
            fields += GENERATED_HEADER_PARAMS_FIELD.substitute(
                type=p.cpp_type, name=p.name,
            )
        if fn.return_type != "void":
            fields += GENERATED_HEADER_PARAMS_FIELD.substitute(
                type=fn.return_type, name="returnValue",
            )
        suffix = _overload_suffix(fn.overload_index)
        parts += GENERATED_HEADER_PARAMS_STRUCT.substitute(
            class_name=cls.name, func_name=fn.name, fields=fields,
            params_struct_suffix=suffix,
        )
    return parts


def _forward_decl_line(kind: str, name: str) -> str:
    """One forward declaration line."""
    if kind == "class":
        return f"class {name};"
    if kind == "struct":
        return f"struct {name};"
    if kind == "template_class":
        return f"template <typename...> class {name};"
    return f"class {name};"


def _build_forward_decl_blocks(forward_decls: list[ForwardDeclInfo]) -> tuple[str, str]:
    """Group forward declarations by namespace and build two blocks:
    - pre_ns: declarations outside the engine namespace (global + other namespaces)
    - engine_ns: declarations inside the DeltaEngine namespace
    """
    from collections import defaultdict
    grouped: dict[tuple[str, ...], list[ForwardDeclInfo]] = defaultdict(list)
    for decl in forward_decls:
        grouped[decl.namespaces].append(decl)

    engine_ns = ("DeltaEngine",)
    pre_ns_lines: list[str] = []
    engine_ns_lines: list[str] = []

    for ns_chain in sorted(grouped.keys(), key=lambda x: (len(x), x)):
        decls = grouped[ns_chain]
        lines = [_forward_decl_line(d.kind, d.name) for d in decls]

        if ns_chain == engine_ns:
            engine_ns_lines.extend(lines)
        elif not ns_chain:
            # Skip global-namespace forward declarations; engine types
            # are in DeltaEngine namespace and global types (IDxcBlob, aiNode, etc.)
            # come from external headers that should be included instead.
            pass
        else:
            indent_lines = ["    " * len(ns_chain) + line for line in lines]
            wrapped: list[str] = []
            for ns in ns_chain:
                indent = "    " * (ns_chain.index(ns))
                wrapped.append(f"{indent}namespace {ns} {{")
            wrapped.extend(indent_lines)
            for ns in reversed(ns_chain):
                indent = "    " * (ns_chain.index(ns))
                wrapped.append(f"{indent}}}")
            pre_ns_lines.extend(wrapped)

    pre_ns_block = "\n".join(pre_ns_lines) + "\n" if pre_ns_lines else ""
    if pre_ns_block:
        pre_ns_block += "\n"
    engine_ns_block = "\n".join(engine_ns_lines) + "\n" if engine_ns_lines else ""
    if engine_ns_block:
        engine_ns_block += "\n"

    return pre_ns_block, engine_ns_block


def generate_header_file(
    classes: list[ClassInfo],
    source_includes: list[str],
    forward_decls: list[ForwardDeclInfo],
) -> str:
    """Build the entire .generated.h for one source file."""
    pre_ns_forward_decls, engine_ns_forward_decls = _build_forward_decl_blocks(forward_decls)

    source_includes_block = "\n".join(source_includes) if source_includes else ""
    if source_includes_block:
        source_includes_block += "\n"

    per_class_content = ""
    for cls in classes:
        params_structs = _params_structs_for_class(cls)
        per_class_content += GENERATED_HEADER_CLASS_BLOCK.substitute(
            class_name=cls.name,
            params_structs=params_structs,
        )

    create_objects_block = ""
    for cls in classes:
        if not cls.is_struct and not cls.is_abstract:
            create_objects_block += GENERATED_HEADER_CREATE_OBJECT.substitute(class_name=cls.name)

    return GENERATED_HEADER_FILE.substitute(
        pre_ns_forward_decls=pre_ns_forward_decls,
        engine_ns_forward_decls=engine_ns_forward_decls,
        source_includes_block=source_includes_block,
        per_class_content=per_class_content,
        create_objects_block=create_objects_block,
    )


# ── source generation ────────────────────────────────────────


def _generate_thunk(cls: ClassInfo, fn: FunctionInfo) -> str:
    thunk_suffix = _overload_suffix(fn.overload_index)
    params_struct_suffix = _overload_suffix(fn.overload_index)
    d = dict(
        class_name=cls.name,
        func_name=fn.name,
        thunk_suffix=thunk_suffix,
        params_struct_suffix=params_struct_suffix,
    )
    has_params = bool(fn.params)
    is_void = fn.return_type == "void"

    if is_void and not has_params:
        return THUNK_VOID_NO_PARAMS.substitute(d)

    extractions = ""
    args = ""
    if has_params:
        for p in fn.params:
            extractions += THUNK_PARAM_EXTRACTION.substitute(
                param_type=p.cpp_type, param_name=p.name,
            )
        args = ", ".join(p.name for p in fn.params)

    d["param_extractions"] = extractions
    d["args"] = args
    d["return_type"] = fn.return_type

    if is_void and has_params:
        return THUNK_WITH_PARAMS_NO_RETURN.substitute(d)
    if not is_void and not has_params:
        return THUNK_NO_PARAMS_WITH_RETURN.substitute(d)
    return THUNK_WITH_PARAMS_WITH_RETURN.substitute(d)


def _dfunction_vector_param_code(p, class_name: str, func_name: str, suffix: str) -> str:
    off = f"offsetof({class_name}_{func_name}_Params{suffix}, {p.name})"
    if p.inner_is_object_ptr:
        return DFUNCTION_PARAM_VECTOR_OBJECT_PTR.substitute(
            pointee_type=p.inner_pointee_type,
            param_name=p.name,
            class_name=class_name,
            func_name=func_name,
            params_struct_suffix=suffix,
        )
    if p.inner_property_class == "DVectorProperty":
        m = re.match(r"^std::vector<(.+)>$", p.inner_cpp_type)
        inner_u = m.group(1) if m else p.inner_cpp_type
        from type_resolver import INNER_TYPE_TO_CPP as _INNER_MAP
        _cpp_to_prop = {v: k for k, v in _INNER_MAP.items()}
        inner_inner = _cpp_to_prop.get(inner_u, "")
        if not inner_inner:
            return ""
        return (
            "        {\n"
            f'            auto* _innerInnerProp = new {inner_inner}("{p.name}_elem_elem", 0);\n'
            f'            auto* _innerProp = new DVectorProperty<{inner_u}>("{p.name}_elem", 0,\n'
            f"                std::unique_ptr<DProperty>(_innerInnerProp));\n"
            f"            fn->AddParam(new DVectorProperty<{p.inner_cpp_type}>(\n"
            f'                "{p.name}",\n'
            f"                {off},\n"
            f"                std::unique_ptr<DProperty>(_innerProp)));\n"
            "        }\n"
        )
    return DFUNCTION_PARAM_VECTOR.substitute(
        inner_property_type=p.inner_property_class,
        inner_cpp_type=p.inner_cpp_type,
        param_name=p.name,
        class_name=class_name,
        func_name=func_name,
        params_struct_suffix=suffix,
    )


def _dfunction_vector_return_code(fn: FunctionInfo, class_name: str, func_name: str, suffix: str) -> str:
    off = f"offsetof({class_name}_{func_name}_Params{suffix}, returnValue)"
    if fn.return_inner_is_object_ptr:
        return DFUNCTION_RETURN_VECTOR_OBJECT_PTR.substitute(
            pointee_type=fn.return_inner_pointee_type,
            class_name=class_name,
            func_name=func_name,
            params_struct_suffix=suffix,
        )
    if fn.return_inner_property_class == "DVectorProperty":
        m = re.match(r"^std::vector<(.+)>$", fn.return_inner_cpp_type)
        inner_u = m.group(1) if m else fn.return_inner_cpp_type
        from type_resolver import INNER_TYPE_TO_CPP as _INNER_MAP
        _cpp_to_prop = {v: k for k, v in _INNER_MAP.items()}
        inner_inner = _cpp_to_prop.get(inner_u, "")
        if not inner_inner:
            return ""
        return (
            "        {\n"
            f'            auto* _innerInnerProp = new {inner_inner}("returnValue_elem_elem", 0);\n'
            f'            auto* _innerProp = new DVectorProperty<{inner_u}>("returnValue_elem", 0,\n'
            f"                std::unique_ptr<DProperty>(_innerInnerProp));\n"
            f"            fn->SetReturnProperty(new DVectorProperty<{fn.return_inner_cpp_type}>(\n"
            f'                "ReturnValue",\n'
            f"                {off},\n"
            f"                std::unique_ptr<DProperty>(_innerProp)));\n"
            "        }\n"
        )
    return DFUNCTION_RETURN_VECTOR.substitute(
        inner_property_type=fn.return_inner_property_class,
        inner_cpp_type=fn.return_inner_cpp_type,
        class_name=class_name,
        func_name=func_name,
        params_struct_suffix=suffix,
    )


def _generate_function_registration(cls: ClassInfo, fn: FunctionInfo) -> str:
    thunk_suffix = _overload_suffix(fn.overload_index)
    params_struct_suffix = _overload_suffix(fn.overload_index)
    is_void = fn.return_type == "void"
    has_params = bool(fn.params)

    if is_void and not has_params:
        return DFUNCTION_VOID_NO_PARAMS.substitute(
            func_name=fn.name,
            thunk_suffix=thunk_suffix,
        )

    param_registrations = ""
    for p in fn.params:
        if p.is_vector:
            param_registrations += _dfunction_vector_param_code(
                p, cls.name, fn.name, params_struct_suffix)
            continue
        m_obj = _OBJECT_PTR_PROP_RE.match(p.property_class)
        if m_obj:
            pointee = m_obj.group(1)
            param_registrations += DFUNCTION_PARAM_OBJECT_PTR.substitute(
                property_type=p.property_class,
                param_name=p.name,
                pointee_type=pointee,
                class_name=cls.name,
                func_name=fn.name,
                params_struct_suffix=params_struct_suffix,
            )
        else:
            param_registrations += DFUNCTION_PARAM.substitute(
                property_type=p.property_class,
                param_name=p.name,
                class_name=cls.name,
                func_name=fn.name,
                params_struct_suffix=params_struct_suffix,
            )

    if is_void and has_params:
        struct_name = f"{cls.name}_{fn.name}_Params{params_struct_suffix}"
        thunk_name = f"{fn.name}_Thunk{thunk_suffix}"
        lines = [
            "    {",
            f'        DFunction* fn = new DFunction("{fn.name}", &{thunk_name}, '
            f'{len(fn.params)}, sizeof({struct_name}), 0);',
        ]
        lines.append(param_registrations.rstrip("\n"))
        lines.append("        cls->AddFunction(fn);")
        lines.append("    }")
        return "\n".join(lines) + "\n"

    return_registration = ""
    if not is_void and fn.return_is_vector:
        return_registration = _dfunction_vector_return_code(
            fn, cls.name, fn.name, params_struct_suffix)
    elif not is_void and fn.return_property_class:
        m_obj = _OBJECT_PTR_PROP_RE.match(fn.return_property_class)
        if m_obj:
            pointee = m_obj.group(1)
            return_registration = DFUNCTION_RETURN_OBJECT_PTR.substitute(
                property_type=fn.return_property_class,
                pointee_type=pointee,
                class_name=cls.name,
                func_name=fn.name,
                params_struct_suffix=params_struct_suffix,
            )
        else:
            return_registration = DFUNCTION_RETURN.substitute(
                property_type=fn.return_property_class,
                class_name=cls.name,
                func_name=fn.name,
                params_struct_suffix=params_struct_suffix,
            )

    return DFUNCTION_WITH_PARAMS.substitute(
        func_name=fn.name,
        class_name=cls.name,
        num_params=len(fn.params),
        param_registrations=param_registrations,
        return_registration=return_registration,
        thunk_suffix=thunk_suffix,
        params_struct_suffix=params_struct_suffix,
    )


def _generate_vector_prop_code(prop, class_name: str) -> str:
    """Generate the AddProperty block for a DVectorProperty field.

    Handles three inner-type categories:
      - Simple value types (DFloatProperty, etc.)
      - Object pointer types (DObjectPtrProperty)
      - Nested vector types (DVectorProperty, recursively)
    """
    if prop.inner_is_object_ptr:
        return DPROPERTY_VECTOR_OBJECT_PTR.substitute(
            pointee_type=prop.inner_pointee_type,
            field_name=prop.name,
            class_name=class_name,
        )
    elif prop.inner_property_class == "DStructProperty":
        return DPROPERTY_VECTOR_DSTRUCT.substitute(
            field_name=prop.name,
            class_name=class_name,
            inner_cpp_type=prop.inner_cpp_type,
            dstruct_type_name=prop.inner_pointee_type or prop.inner_cpp_type,
        )
    elif prop.inner_property_class == "DVectorProperty":
        # Nested vector: the inner_cpp_type is std::vector<U>.
        # We need an inner DVectorProperty with its own simple inner prop.
        # Extract U from "std::vector<U>".
        import re as _re
        m = _re.match(r'^std::vector<(.+)>$', prop.inner_cpp_type)
        inner_u = m.group(1) if m else prop.inner_cpp_type
        # Build the inner-inner property type name from INNER_TYPE_TO_CPP reverse map.
        from type_resolver import INNER_TYPE_TO_CPP as _INNER_MAP
        _cpp_to_prop = {v: k for k, v in _INNER_MAP.items()}
        inner_inner_prop_class = _cpp_to_prop.get(inner_u, "")
        if not inner_inner_prop_class:
            return ""
        return (
            f"    {{\n"
            f'        auto* _innerInnerProp = new {inner_inner_prop_class}("{prop.name}_elem_elem", 0);\n'
            f'        auto* _innerProp = new DVectorProperty<{inner_u}>("{prop.name}_elem", 0,\n'
            f'            std::unique_ptr<DProperty>(_innerInnerProp));\n'
            f'        cls->AddProperty(new DVectorProperty<{prop.inner_cpp_type}>(\n'
            f'            "{prop.name}",\n'
            f'            offsetof({class_name}, {prop.name}),\n'
            f'            std::unique_ptr<DProperty>(_innerProp)));\n'
            f"    }}\n"
        )
    else:
        return DPROPERTY_VECTOR.substitute(
            inner_property_type=prop.inner_property_class,
            inner_cpp_type=prop.inner_cpp_type,
            field_name=prop.name,
            class_name=class_name,
        )


def _generate_class_registration(cls: ClassInfo) -> str:
    """Generate the registration function body for one class/struct."""
    parts: list[str] = []

    if cls.is_struct:
        super_name = cls.base_name or ""
        parts.append(DSTRUCT_REGISTRATION_BEGIN.substitute(
            class_name=cls.name,
            super_name=super_name,
        ))
    elif cls.is_abstract:
        super_name = cls.base_name or ""
        parts.append(DCLASS_REGISTRATION_BEGIN_ABSTRACT.substitute(
            class_name=cls.name,
            super_name=super_name,
        ))
    else:
        super_name = cls.base_name or ""
        parts.append(DCLASS_REGISTRATION_BEGIN.substitute(
            class_name=cls.name,
            super_name=super_name,
        ))

    for prop in cls.properties:
        if prop.is_vector:
            code = _generate_vector_prop_code(prop, cls.name)
            if code:
                parts.append(code)
        elif prop.is_dstruct:
            parts.append(DPROPERTY_DSTRUCT.substitute(
                field_name=prop.name,
                class_name=cls.name,
                dstruct_type_name=prop.dstruct_type_name,
            ))
        elif prop.is_object_ptr:
            parts.append(DPROPERTY_OBJECT_PTR.substitute(
                pointee_type=prop.pointee_type,
                field_name=prop.name,
                class_name=cls.name,
            ))
        elif prop.metadata:
            parts.append(DPROPERTY_WITH_META.substitute(
                property_type=prop.property_class,
                field_name=prop.name,
                class_name=cls.name,
                meta_init=_format_meta_init(prop.metadata),
            ))
        else:
            parts.append(DPROPERTY.substitute(
                property_type=prop.property_class,
                field_name=prop.name,
                class_name=cls.name,
            ))

    for fn in cls.functions:
        parts.append(_generate_function_registration(cls, fn))

    if cls.is_struct:
        parts.append(DSTRUCT_REGISTRATION_END)
    else:
        parts.append(DCLASS_REGISTRATION_END)

    return "\n".join(parts)


def _generate_class_footer(cls: ClassInfo) -> str:
    """Generate the footer (CreateDObject + static registration) for one class."""
    if cls.is_struct or cls.is_abstract:
        return FILE_FOOTER_NO_CREATE.substitute(class_name=cls.name)
    return FILE_FOOTER_CLASS.substitute(class_name=cls.name)


def generate_source_file(classes: list[ClassInfo], header_stem: str, type_to_header: dict[str, str]) -> str:
    """Generate the entire .generated.cpp for all classes in one header file.
    Emits a single #include block, then per-class thunks/registration/footers."""
    parts: list[str] = []

    # Collect unique include paths (dedup)
    source_includes = set()
    for cls in classes:
        source_includes.add(f'#include "{cls.include_path}{header_stem}.h"')
    source_header_include = "\n".join(sorted(source_includes))

    cpp_full_includes = _collect_cpp_full_includes(classes, header_stem, type_to_header)
    for cls in classes:
        for prop in cls.properties:
            if prop.property_class in EXTRA_PROPERTY_HEADERS:
                cpp_full_includes.add(EXTRA_PROPERTY_HEADERS[prop.property_class])
    cpp_full_includes_block = "\n".join(sorted(f'#include "{p}"' for p in cpp_full_includes))
    if cpp_full_includes_block:
        cpp_full_includes_block += "\n"

    parts.append(FILE_HEADER.substitute(
        header_stem=header_stem,
        source_header_include=source_header_include,
        cpp_full_includes=cpp_full_includes_block,
    ))

    for cls in classes:
        for fn in cls.functions:
            parts.append(_generate_thunk(cls, fn))

    for cls in classes:
        parts.append(_generate_class_registration(cls))

    for cls in classes:
        parts.append(_generate_class_footer(cls))

    return "\n".join(parts)
