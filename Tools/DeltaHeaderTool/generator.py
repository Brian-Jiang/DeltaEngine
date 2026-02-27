from __future__ import annotations

from parser import ClassInfo, FunctionInfo
from templates import (
    FILE_HEADER,
    FILE_FOOTER,
    THUNK_VOID_NO_PARAMS,
    THUNK_NO_PARAMS_WITH_RETURN,
    THUNK_WITH_PARAMS_NO_RETURN,
    THUNK_WITH_PARAMS_WITH_RETURN,
    THUNK_PARAM_EXTRACTION,
    DCLASS_REGISTRATION_BEGIN,
    DCLASS_REGISTRATION_END,
    DPROPERTY,
    DPROPERTY_OBJECT_PTR,
    DFUNCTION_VOID_NO_PARAMS,
    DFUNCTION_WITH_PARAMS,
    DFUNCTION_PARAM,
    DFUNCTION_RETURN,
    GENERATED_HEADER_FILE,
    GENERATED_HEADER_PARAMS_STRUCT,
    GENERATED_HEADER_PARAMS_FIELD,
)


# ── header generation ────────────────────────────────────────


def generate_header(cls: ClassInfo) -> str:
    params_structs = ""
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
        params_structs += GENERATED_HEADER_PARAMS_STRUCT.substitute(
            class_name=cls.name, func_name=fn.name, fields=fields,
        )

    return GENERATED_HEADER_FILE.substitute(
        class_name=cls.name,
        params_structs=params_structs,
    )


# ── source generation ────────────────────────────────────────


def _generate_thunk(cls: ClassInfo, fn: FunctionInfo) -> str:
    d = dict(class_name=cls.name, func_name=fn.name)
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


def _generate_function_registration(cls: ClassInfo, fn: FunctionInfo) -> str:
    is_void = fn.return_type == "void"
    has_params = bool(fn.params)

    if is_void and not has_params:
        return DFUNCTION_VOID_NO_PARAMS.substitute(
            func_name=fn.name,
        )

    param_registrations = ""
    for p in fn.params:
        param_registrations += DFUNCTION_PARAM.substitute(
            property_type=p.property_class,
            param_name=p.name,
            class_name=cls.name,
            func_name=fn.name,
        )

    if is_void and has_params:
        # DFUNCTION_WITH_PARAMS hardcodes offsetof(..., returnValue) which
        # doesn't exist for void functions.  Build the block directly.
        struct_name = f"{cls.name}_{fn.name}_Params"
        lines = [
            "    {",
            f'        DFunction* fn = new DFunction("{fn.name}", &{fn.name}_Thunk, '
            f'{len(fn.params)}, sizeof({struct_name}), 0);',
        ]
        lines.append(param_registrations.rstrip("\n"))
        lines.append("        cls->AddFunction(fn);")
        lines.append("    }")
        return "\n".join(lines) + "\n"

    return_registration = ""
    if not is_void:
        return_registration = DFUNCTION_RETURN.substitute(
            property_type=fn.return_property_class,
            class_name=cls.name,
            func_name=fn.name,
        )

    return DFUNCTION_WITH_PARAMS.substitute(
        func_name=fn.name,
        class_name=cls.name,
        num_params=len(fn.params),
        param_registrations=param_registrations,
        return_registration=return_registration,
    )


def generate_source(cls: ClassInfo) -> str:
    parts: list[str] = []

    parts.append(FILE_HEADER.substitute(
        class_name=cls.name,
        include_path=cls.include_path,
    ))

    for fn in cls.functions:
        parts.append(_generate_thunk(cls, fn))

    parts.append(DCLASS_REGISTRATION_BEGIN.substitute(class_name=cls.name))

    for prop in cls.properties:
        if prop.is_object_ptr:
            parts.append(DPROPERTY_OBJECT_PTR.substitute(
                pointee_type=prop.pointee_type,
                field_name=prop.name,
                class_name=cls.name,
            ))
        else:
            parts.append(DPROPERTY.substitute(
                property_type=prop.property_class,
                field_name=prop.name,
                class_name=cls.name,
            ))

    for fn in cls.functions:
        parts.append(_generate_function_registration(cls, fn))

    parts.append(DCLASS_REGISTRATION_END)

    parts.append(FILE_FOOTER.substitute(class_name=cls.name))

    return "\n".join(parts)
