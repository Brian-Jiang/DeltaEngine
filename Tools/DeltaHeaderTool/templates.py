from string import Template

# ============================================================
# FILE LEVEL (source .generated.cpp)
# ============================================================

# Emitted once per .generated.cpp for all classes in the file.
# ${header_stem} is the header file stem (e.g. "TestComponent")
# ${source_header_include} is the #include for the source header
FILE_HEADER = Template("""\
#include "${header_stem}.generated.h"

${source_header_include}

#include "Runtime/Reflection/DStruct.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/DFunction.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

using namespace DeltaEngine;
using namespace DeltaEngine::Reflection::Private;
""")

# Footer for a non-abstract DCLASS: CreateDObject specialization + static registration
FILE_FOOTER_CLASS = Template("""\
template <>
${class_name}* DeltaEngine::CreateDObject<${class_name}>()
{
    return GetReflectionRegistry().CreateObject<${class_name}>("${class_name}");
}

static ReflectionRegistration registration_${class_name}(
    &Reflection::Private::ReflectionRegister_${class_name}::ReflectionRegisterFn_${class_name});
""")

# Footer for abstract DCLASS or DSTRUCT: no CreateDObject, just static registration
FILE_FOOTER_NO_CREATE = Template("""\
static ReflectionRegistration registration_${class_name}(
    &Reflection::Private::ReflectionRegister_${class_name}::ReflectionRegisterFn_${class_name});
""")

# ============================================================
# THUNKS
# ============================================================

THUNK_VOID_NO_PARAMS = Template("""\
static void ${func_name}_Thunk${thunk_suffix}(DObject* instance, void* params)
{
    static_cast<${class_name}*>(instance)->${func_name}();
}
""")

THUNK_NO_PARAMS_WITH_RETURN = Template("""\
static void ${func_name}_Thunk${thunk_suffix}(DObject* instance, void* params)
{
    ${class_name}_${func_name}_Params${params_struct_suffix}* typedParams = static_cast<${class_name}_${func_name}_Params${params_struct_suffix}*>(params);
    ${return_type} ret = static_cast<${class_name}*>(instance)->${func_name}();
    typedParams->returnValue = ret;
}
""")

THUNK_WITH_PARAMS_NO_RETURN = Template("""\
static void ${func_name}_Thunk${thunk_suffix}(DObject* instance, void* params)
{
    ${class_name}_${func_name}_Params${params_struct_suffix}* typedParams = static_cast<${class_name}_${func_name}_Params${params_struct_suffix}*>(params);
${param_extractions}\
    static_cast<${class_name}*>(instance)->${func_name}(${args});
}
""")

THUNK_WITH_PARAMS_WITH_RETURN = Template("""\
static void ${func_name}_Thunk${thunk_suffix}(DObject* instance, void* params)
{
    ${class_name}_${func_name}_Params${params_struct_suffix}* typedParams = static_cast<${class_name}_${func_name}_Params${params_struct_suffix}*>(params);
${param_extractions}\
    ${return_type} ret = static_cast<${class_name}*>(instance)->${func_name}(${args});
    typedParams->returnValue = ret;
}
""")

THUNK_PARAM_EXTRACTION = Template("""\
    ${param_type} ${param_name} = typedParams->${param_name};
""")

# ============================================================
# DCLASS REGISTRATION
# ============================================================

DCLASS_REGISTRATION_BEGIN = Template("""\
void Reflection::Private::ReflectionRegister_${class_name}::ReflectionRegisterFn_${class_name}()
{
    DClass* cls = new DClass("${class_name}",
                             "${super_name}",
                             sizeof(${class_name}),
                             alignof(${class_name}),
                             [](void* address) { new (address) ${class_name}(); },
                             [](void* address) { static_cast<${class_name}*>(address)->~${class_name}(); },
                             [](void* dest, const void* src) { new (dest) ${class_name}(*static_cast<const ${class_name}*>(src)); },
                             nullptr
    );
""")

DCLASS_REGISTRATION_BEGIN_ABSTRACT = Template("""\
void Reflection::Private::ReflectionRegister_${class_name}::ReflectionRegisterFn_${class_name}()
{
    DClass* cls = new DClass("${class_name}",
                             "${super_name}",
                             sizeof(${class_name}),
                             alignof(${class_name}),
                             nullptr,
                             nullptr,
                             nullptr,
                             nullptr,
                             true
    );
""")

DCLASS_REGISTRATION_END = """\
    GetReflectionRegistry().RegisterDClass(cls);
}
"""

# ============================================================
# DSTRUCT REGISTRATION
# ============================================================

DSTRUCT_REGISTRATION_BEGIN = Template("""\
void Reflection::Private::ReflectionRegister_${class_name}::ReflectionRegisterFn_${class_name}()
{
    DStruct* cls = new DStruct("${class_name}",
                               "${super_name}",
                               sizeof(${class_name}),
                               alignof(${class_name})
    );
""")

DSTRUCT_REGISTRATION_END = """\
    GetReflectionRegistry().RegisterDStruct(cls);
}
"""

# ============================================================
# DPROPERTY REGISTRATION  (one per DPROPERTY field)
# ============================================================

DPROPERTY = Template("""\
    cls->AddProperty(new ${property_type}(
        "${field_name}",
        offsetof(${class_name}, ${field_name})));
""")

DPROPERTY_OBJECT_PTR = Template("""\
    cls->AddProperty(new DObjectPtrProperty<${pointee_type}>(
        "${field_name}",
        "${pointee_type}",
        offsetof(${class_name}, ${field_name})));
""")

DPROPERTY_SHARED_PTR = Template("""\
    cls->AddProperty(new DSharedObjectPtrProperty<${pointee_type}>(
        "${field_name}",
        "${pointee_type}",
        offsetof(${class_name}, ${field_name})));
""")

# ============================================================
# DFUNCTION REGISTRATION
# ============================================================

DFUNCTION_VOID_NO_PARAMS = Template("""\
    {
        DFunction* fn = new DFunction("${func_name}", &${func_name}_Thunk${thunk_suffix}, 0, 0, 0);
        cls->AddFunction(fn);
    }
""")

DFUNCTION_WITH_PARAMS = Template("""\
    {
        DFunction* fn = new DFunction("${func_name}", &${func_name}_Thunk${thunk_suffix}, ${num_params}, sizeof(${class_name}_${func_name}_Params${params_struct_suffix}), offsetof(${class_name}_${func_name}_Params${params_struct_suffix}, returnValue));
${param_registrations}\
${return_registration}\
        cls->AddFunction(fn);
    }
""")

DFUNCTION_PARAM = Template("""\
        fn->AddParam(new ${property_type}("${param_name}", offsetof(${class_name}_${func_name}_Params${params_struct_suffix}, ${param_name})));
""")

DFUNCTION_PARAM_SHARED_PTR = Template("""\
        fn->AddParam(new ${property_type}("${param_name}", "${pointee_type}", offsetof(${class_name}_${func_name}_Params${params_struct_suffix}, ${param_name})));
""")

DFUNCTION_RETURN = Template("""\
        fn->SetReturnProperty(new ${property_type}("ReturnValue", offsetof(${class_name}_${func_name}_Params${params_struct_suffix}, returnValue)));
""")

DFUNCTION_RETURN_SHARED_PTR = Template("""\
        fn->SetReturnProperty(new ${property_type}("ReturnValue", "${pointee_type}", offsetof(${class_name}_${func_name}_Params${params_struct_suffix}, returnValue)));
""")


# ============================================================
# GENERATED HEADER (file-level: forward decls, includes, per-class content)
# ============================================================

# The header template uses split forward decl blocks:
# ${pre_ns_forward_decls} goes before the engine namespace (global/other namespaces)
# ${engine_ns_forward_decls} goes inside DELTA_ENGINE_NS_BEGIN
GENERATED_HEADER_FILE = Template("""\
#pragma once
#include "EngineIncludes.h"

// Includes from source header
${source_includes_block}

${pre_ns_forward_decls}\
DELTA_ENGINE_NS_BEGIN

// Forward declarations (auto-generated by DeltaHeaderTool)
${engine_ns_forward_decls}
namespace Reflection {
namespace Private {

${per_class_content}\
} // namespace Private
} // namespace Reflection

${create_objects_block}\
DELTA_ENGINE_NS_END
""")

GENERATED_HEADER_CLASS_BLOCK = Template("""\
${params_structs}\
class ReflectionRegister_${class_name}
{
public:
    static void ReflectionRegisterFn_${class_name}();
};

""")

GENERATED_HEADER_CREATE_OBJECT = Template("""\
template <>
${class_name}* CreateDObject<${class_name}>();
""")

GENERATED_HEADER_PARAMS_STRUCT = Template("""\
struct ${class_name}_${func_name}_Params${params_struct_suffix}
{
${fields}\
};

""")

GENERATED_HEADER_PARAMS_FIELD = Template("""\
    ${type} ${name};
""")
