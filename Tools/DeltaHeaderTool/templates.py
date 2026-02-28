from string import Template

# ============================================================
# FILE LEVEL
# ============================================================

FILE_HEADER = Template("""\
#include "${class_name}.generated.h"

#include "${include_path}${class_name}.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/DFunction.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

using namespace DeltaEngine;
using namespace DeltaEngine::Reflection::Private;
""")

FILE_FOOTER = Template("""\
template <>
${class_name}* DeltaEngine::CreateDObject<${class_name}>()
{
    return GetReflectionRegistry().CreateObject<${class_name}>("${class_name}");
}

static ReflectionRegistration registration_${class_name}(
    &Reflection::Private::ReflectionRegister_${class_name}::ReflectionRegisterFn_${class_name});
""")

# ============================================================
# PARAMS STRUCTS  (top of file, before thunks)
# ============================================================

# For functions that have params and/or a return value (params_struct_suffix is _2, _3, ... for overloads)
PARAMS_STRUCT = Template("""\
struct ${class_name}_${func_name}_Params${params_struct_suffix}
{
${fields}\
};
""")

PARAMS_STRUCT_FIELD = Template("""\
    ${type} ${name};
""")

# ============================================================
# THUNKS
# ============================================================

# no params, no return (thunk_suffix is _2, _3, ... for overloads)
THUNK_VOID_NO_PARAMS = Template("""\
static void ${func_name}_Thunk${thunk_suffix}(DObject* instance, void* params)
{
    static_cast<${class_name}*>(instance)->${func_name}();
}
""")

# no params, with return
THUNK_NO_PARAMS_WITH_RETURN = Template("""\
static void ${func_name}_Thunk${thunk_suffix}(DObject* instance, void* params)
{
    ${class_name}_${func_name}_Params${params_struct_suffix}* typedParams = static_cast<${class_name}_${func_name}_Params${params_struct_suffix}*>(params);
    ${return_type} ret = static_cast<${class_name}*>(instance)->${func_name}();
    typedParams->returnValue = ret;
}
""")

# with params, no return
THUNK_WITH_PARAMS_NO_RETURN = Template("""\
static void ${func_name}_Thunk${thunk_suffix}(DObject* instance, void* params)
{
    ${class_name}_${func_name}_Params${params_struct_suffix}* typedParams = static_cast<${class_name}_${func_name}_Params${params_struct_suffix}*>(params);
${param_extractions}\
    static_cast<${class_name}*>(instance)->${func_name}(${args});
}
""")

# with params, with return
THUNK_WITH_PARAMS_WITH_RETURN = Template("""\
static void ${func_name}_Thunk${thunk_suffix}(DObject* instance, void* params)
{
    ${class_name}_${func_name}_Params${params_struct_suffix}* typedParams = static_cast<${class_name}_${func_name}_Params${params_struct_suffix}*>(params);
${param_extractions}\
    ${return_type} ret = static_cast<${class_name}*>(instance)->${func_name}(${args});
    typedParams->returnValue = ret;
}
""")

# one line per param extraction, used to build ${param_extractions}
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

DCLASS_REGISTRATION_END = """\
    GetReflectionRegistry().RegisterDClass(cls);
}
"""

# ============================================================
# DPROPERTY REGISTRATION  (one per DPROPERTY field)
# ============================================================

# Standard typed properties
DPROPERTY = Template("""\
    cls->AddProperty(new ${property_type}(
        "${field_name}",
        offsetof(${class_name}, ${field_name})));
""")

# Object pointer property (needs pointee class name)
DPROPERTY_OBJECT_PTR = Template("""\
    cls->AddProperty(new DObjectPtrProperty<${pointee_type}>(
        "${field_name}",
        "${pointee_type}",
        offsetof(${class_name}, ${field_name})));
""")

# ============================================================
# DFUNCTION REGISTRATION
# ============================================================

# No params, no return value (thunk_suffix is _2, _3, ... for overloads)
DFUNCTION_VOID_NO_PARAMS = Template("""\
    {
        DFunction* fn = new DFunction("${func_name}", &${func_name}_Thunk${thunk_suffix}, 0, 0, 0);
        cls->AddFunction(fn);
    }
""")

# Has params and/or return value
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

DFUNCTION_RETURN = Template("""\
        fn->SetReturnProperty(new ${property_type}("ReturnValue", offsetof(${class_name}_${func_name}_Params${params_struct_suffix}, returnValue)));
""")


# ============================================================
# GENERATED HEADER (file-level: forward decls, includes, per-class content)
# ============================================================

GENERATED_HEADER_FILE = Template("""\
#pragma once
#include "EngineIncludes.h"

// Includes from source header
${source_includes_block}

DELTA_ENGINE_NS_BEGIN

// Forward declarations (auto-generated by DeltaHeaderTool)
${forward_decls_block}

namespace Reflection {
namespace Private {

${per_class_content}\
} // namespace Private
} // namespace Reflection

${create_objects_block}\
DELTA_ENGINE_NS_END
""")

# Per-class block: params structs + ReflectionRegister class + CreateDObject declaration
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

# Only emitted for functions that have params or a return value (params_struct_suffix is _2, _3, ... for overloads)
GENERATED_HEADER_PARAMS_STRUCT = Template("""\
struct ${class_name}_${func_name}_Params${params_struct_suffix}
{
${fields}\
};

""")

GENERATED_HEADER_PARAMS_FIELD = Template("""\
    ${type} ${name};
""")
