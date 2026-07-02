from string import Template

# ============================================================
# FILE LEVEL (source .generated.cpp)
# ============================================================

# Emitted once per .generated.cpp for all classes in the file.
# ${header_stem} is the header file stem (e.g. "TestComponent")
# ${source_header_include} is the #include for the source header
# ${cpp_full_includes} is full includes for ObjectPtr DObject-derived types
FILE_HEADER = Template("""\
#include "${header_stem}.generated.h"

${source_header_include}

${cpp_full_includes}
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
DELTAENGINE_API ${class_name}* DeltaEngine::CreateDObject<${class_name}>()
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

DCLASS_SET_METADATA = Template("""\
    cls->SetMetadata(${meta_init});
""")

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

DPROPERTY_WITH_META = Template("""\
    {
        auto* _prop = new ${property_type}(
            "${field_name}",
            offsetof(${class_name}, ${field_name}));
        _prop->SetMetadata(${meta_init});
        cls->AddProperty(_prop);
    }
""")

DPROPERTY_OBJECT_PTR = Template("""\
    cls->AddProperty(new DObjectPtrProperty<${pointee_type}>(
        "${field_name}",
        "${pointee_type}",
        offsetof(${class_name}, ${field_name})));
""")

DPROPERTY_VECTOR = Template("""\
    {
        auto* _innerProp = new ${inner_property_type}("${field_name}_elem", 0);
        cls->AddProperty(new DVectorProperty<${inner_cpp_type}>(
            "${field_name}",
            offsetof(${class_name}, ${field_name}),
            std::unique_ptr<DProperty>(_innerProp)));
    }
""")

DPROPERTY_VECTOR_OBJECT_PTR = Template("""\
    {
        auto* _innerProp = new DObjectPtrProperty<${pointee_type}>("${field_name}_elem", "${pointee_type}", 0);
        cls->AddProperty(new DVectorProperty<${pointee_type}*>(
            "${field_name}",
            offsetof(${class_name}, ${field_name}),
            std::unique_ptr<DProperty>(_innerProp)));
    }
""")

DPROPERTY_DSTRUCT = Template("""\
    cls->AddProperty(new DStructProperty(
        "${field_name}",
        offsetof(${class_name}, ${field_name}),
        sizeof(${dstruct_type_name}),
        "${dstruct_type_name}"));
""")

DPROPERTY_VECTOR_DSTRUCT = Template("""\
    {
        auto* _innerProp = new DStructProperty(
            "${field_name}_elem",
            0,
            sizeof(${dstruct_type_name}),
            "${dstruct_type_name}");
        cls->AddProperty(new DVectorProperty<${inner_cpp_type}>(
            "${field_name}",
            offsetof(${class_name}, ${field_name}),
            std::unique_ptr<DProperty>(_innerProp)));
    }
""")

# ============================================================
# DFUNCTION REGISTRATION
# ============================================================

DFUNCTION_VOID_NO_PARAMS = Template("""\
    {
        DFunction* fn = new DFunction("${func_name}", &${func_name}_Thunk${thunk_suffix}, 0, 0, 0);
${metadata_call}\
        cls->AddFunction(fn);
    }
""")

DFUNCTION_WITH_PARAMS = Template("""\
    {
        DFunction* fn = new DFunction("${func_name}", &${func_name}_Thunk${thunk_suffix}, ${num_params}, sizeof(${class_name}_${func_name}_Params${params_struct_suffix}), offsetof(${class_name}_${func_name}_Params${params_struct_suffix}, returnValue));
${param_registrations}\
${return_registration}\
${metadata_call}\
        cls->AddFunction(fn);
    }
""")

DFUNCTION_SET_METADATA = Template("""\
        fn->SetMetadata(${meta_init});
""")

DFUNCTION_PARAM = Template("""\
        fn->AddParam(new ${property_type}("${param_name}", offsetof(${class_name}_${func_name}_Params${params_struct_suffix}, ${param_name})));
""")

DFUNCTION_PARAM_OBJECT_PTR = Template("""\
        fn->AddParam(new ${property_type}("${param_name}", "${pointee_type}", offsetof(${class_name}_${func_name}_Params${params_struct_suffix}, ${param_name})));
""")

DFUNCTION_PARAM_VECTOR = Template("""\
        {
            auto* _innerProp = new ${inner_property_type}("${param_name}_elem", 0);
            fn->AddParam(new DVectorProperty<${inner_cpp_type}>(
                "${param_name}",
                offsetof(${class_name}_${func_name}_Params${params_struct_suffix}, ${param_name}),
                std::unique_ptr<DProperty>(_innerProp)));
        }
""")

DFUNCTION_PARAM_VECTOR_OBJECT_PTR = Template("""\
        {
            auto* _innerProp = new DObjectPtrProperty<${pointee_type}>("${param_name}_elem", "${pointee_type}", 0);
            fn->AddParam(new DVectorProperty<${pointee_type}*>(
                "${param_name}",
                offsetof(${class_name}_${func_name}_Params${params_struct_suffix}, ${param_name}),
                std::unique_ptr<DProperty>(_innerProp)));
        }
""")

DFUNCTION_RETURN = Template("""\
        fn->SetReturnProperty(new ${property_type}("ReturnValue", offsetof(${class_name}_${func_name}_Params${params_struct_suffix}, returnValue)));
""")

DFUNCTION_RETURN_OBJECT_PTR = Template("""\
        fn->SetReturnProperty(new ${property_type}("ReturnValue", "${pointee_type}", offsetof(${class_name}_${func_name}_Params${params_struct_suffix}, returnValue)));
""")

DFUNCTION_RETURN_VECTOR = Template("""\
        {
            auto* _innerProp = new ${inner_property_type}("returnValue_elem", 0);
            fn->SetReturnProperty(new DVectorProperty<${inner_cpp_type}>(
                "ReturnValue",
                offsetof(${class_name}_${func_name}_Params${params_struct_suffix}, returnValue),
                std::unique_ptr<DProperty>(_innerProp)));
        }
""")

DFUNCTION_RETURN_VECTOR_OBJECT_PTR = Template("""\
        {
            auto* _innerProp = new DObjectPtrProperty<${pointee_type}>("returnValue_elem", "${pointee_type}", 0);
            fn->SetReturnProperty(new DVectorProperty<${pointee_type}*>(
                "ReturnValue",
                offsetof(${class_name}_${func_name}_Params${params_struct_suffix}, returnValue),
                std::unique_ptr<DProperty>(_innerProp)));
        }
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
DELTAENGINE_API ${class_name}* CreateDObject<${class_name}>();
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

# ============================================================
# DYNAMIC DELEGATE CODEGEN
# ============================================================

DELEGATE_PARAMS_STRUCT = Template("""\
struct ${delegate_name}_Params
{
${fields}\
};

""")

DELEGATE_PARAM_ASSIGNMENT = Template("""\
    params.${param_name} = ${param_name};
""")

DELEGATE_BROADCAST_WITH_PARAMS = Template("""\
void ${delegate_name}::Broadcast(${param_declarations}) const
{
    ${delegate_name}_Params params;
${param_assignments}\
    BroadcastWithParams(&params, ${param_count});
}
""")

DELEGATE_BROADCAST_NO_PARAMS = Template("""\
void ${delegate_name}::Broadcast() const
{
    BroadcastWithParams(nullptr, 0);
}
""")

DELEGATE_EXECUTE_WITH_PARAMS = Template("""\
void ${delegate_name}::Execute(${param_declarations}) const
{
    ${delegate_name}_Params params;
${param_assignments}\
    ExecuteWithParams(&params, ${param_count});
}
""")

DELEGATE_EXECUTE_NO_PARAMS = Template("""\
void ${delegate_name}::Execute() const
{
    ExecuteWithParams(nullptr, 0);
}
""")

DPROPERTY_DELEGATE = Template("""\
    cls->AddProperty(new DDelegateProperty(
        "${field_name}",
        offsetof(${class_name}, ${field_name})));
""")

# ============================================================
# DENUM REGISTRATION
# ============================================================

DENUM_ADD_ENTRY = Template('    e->AddEntry("${entry_name}", ${entry_value});')

DENUM_REGISTRATION = Template("""\
static void Register_${enum_name}()
{
    auto* e = new DEnum("${enum_name}", "${underlying_type}");
${entries}
    GetReflectionRegistry().RegisterDEnum(e);
}

static ReflectionRegistration registration_${enum_name}(&Register_${enum_name});
""")

DPROPERTY_ENUM = Template("""\
    cls->AddProperty(new DEnumProperty<${enum_type}>(
        "${field_name}",
        offsetof(${class_name}, ${field_name}),
        "${enum_type_name}"));""")

DPROPERTY_ENUM_WITH_META = Template("""\
    {
        auto* _prop = new DEnumProperty<${enum_type}>(
            "${field_name}",
            offsetof(${class_name}, ${field_name}),
            "${enum_type_name}");
        _prop->SetMetadata(${meta_init});
        cls->AddProperty(_prop);
    }""")
