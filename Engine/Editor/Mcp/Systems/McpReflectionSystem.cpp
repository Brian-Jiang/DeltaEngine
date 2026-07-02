#include "McpReflectionSystem.h"

#include "Mcp/McpRegistry.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/DFunction.h"
#include "Runtime/Reflection/DEnumProperty.h"

#include <unordered_set>

using namespace DeltaEngine;

static const char* PropertyTypeName(EPropertyType t)
{
    switch (t)
    {
    case EPropertyType::Float:      return "float";
    case EPropertyType::Int:        return "int";
    case EPropertyType::Bool:       return "bool";
    case EPropertyType::Double:     return "double";
    case EPropertyType::String:     return "string";
    case EPropertyType::FilesystemPath: return "path";
    case EPropertyType::Vector3:    return "Vector3";
    case EPropertyType::Quaternion: return "Quaternion";
    case EPropertyType::Float4:     return "Float4";
    case EPropertyType::Float4x4:   return "Float4x4";
    case EPropertyType::BoundingBox: return "BoundingBox";
    case EPropertyType::ObjectPtr:  return "ObjectPtr";
    case EPropertyType::BulkData:   return "BulkData";
    case EPropertyType::Vector:     return "Vector";
    case EPropertyType::Struct:     return "Struct";
    case EPropertyType::Delegate:   return "Delegate";
    case EPropertyType::Enum:       return "enum";
    default:
        DELTA_CHECK_MSG(false,
            "EPropertyType {} not enumerated for MCP property schema",
            static_cast<int>(t));
        return "unknown";
    }
}

static nlohmann::json MakeError(const std::string& msg)
{
    return { {"ok", false}, {"error", msg} };
}

static nlohmann::json SerializePropertySchema(const DProperty* p)
{
    nlohmann::json entry = {
        {"name", p->GetName()},
        {"cpp_type", p->GetType()},
        {"type", PropertyTypeName(p->GetPropertyType())}
    };

    if (p->GetPropertyType() == EPropertyType::Enum)
    {
        const auto* enumProp = static_cast<const DEnumPropertyBase*>(p);
        if (const DEnum* schema = enumProp->GetEnumSchema())
        {
            entry["enum_name"] = schema->GetName();
            nlohmann::json values = nlohmann::json::array();
            for (const DEnumEntry& e : schema->GetEntries())
                values.push_back({ {"name", e.name}, {"value", e.value} });
            entry["values"] = std::move(values);
        }
    }

    return entry;
}

static nlohmann::json SerializeFunctionSchema(const DFunction* f)
{
    nlohmann::json params = nlohmann::json::array();
    for (const DProperty* p : f->GetParams())
    {
        params.push_back({
            {"name", p->GetName()},
            {"cpp_type", p->GetType()},
            {"type", PropertyTypeName(p->GetPropertyType())}
        });
    }

    nlohmann::json entry;
    entry["name"] = f->GetName();
    entry["params"] = std::move(params);

    if (f->HasReturnValue())
    {
        const DProperty* ret = f->GetReturnProperty();
        entry["return_type"] = ret->GetType();
    }

    return entry;
}

// ─── Registration ───────────────────────────────────────────────────────────

void McpReflectionSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterQuery("reflection", "classes",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryClasses(c, p); });
    registry.RegisterQuery("reflection", "class_schema",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryClassSchema(c, p); });
    registry.RegisterQuery("reflection", "inheritance_chain",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryInheritanceChain(c, p); });
    registry.RegisterQuery("reflection", "find_classes_with_property",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryFindClassesWithProperty(c, p); });
}

// ─── classes ────────────────────────────────────────────────────────────────

nlohmann::json McpReflectionSystem::QueryClasses(EditorCore&, const nlohmann::json& params)
{
    std::string baseFilter = params.value("base_class_filter", "");
    bool includeAbstract = params.value("include_abstract", true);

    const DClass* baseClass = nullptr;
    if (!baseFilter.empty())
    {
        baseClass = GetReflectionRegistry().FindClassByName(baseFilter);
        if (!baseClass)
            return MakeError("unknown base class: " + baseFilter);
    }

    nlohmann::json arr = nlohmann::json::array();
    for (auto& [name, dclass] : GetReflectionRegistry().GetAllClasses())
    {
        if (!includeAbstract && dclass->IsAbstract())
            continue;
        if (baseClass && !dclass->IsChildOf(baseClass))
            continue;

        arr.push_back({
            {"name", dclass->GetName()},
            {"super", dclass->GetSuperName()},
            {"is_abstract", dclass->IsAbstract()}
        });
    }

    return { {"ok", true}, {"classes", std::move(arr)} };
}

// ─── class_schema ───────────────────────────────────────────────────────────

nlohmann::json McpReflectionSystem::QueryClassSchema(EditorCore&, const nlohmann::json& params)
{
    if (!params.contains("class_name"))
        return MakeError("missing required param: class_name");

    std::string className = params["class_name"].get<std::string>();
    DClass* dclass = GetReflectionRegistry().FindClassByName(className);
    if (!dclass)
        return MakeError("unknown class: " + className);

    bool includeInherited = params.value("include_inherited", true);
    bool includeFunctions = params.value("include_functions", false);

    nlohmann::json properties = nlohmann::json::array();
    if (includeInherited)
    {
        for (const DProperty* p = dclass->GetProperties(); p; p = p->GetHierarchyNext())
            properties.push_back(SerializePropertySchema(p));
    }
    else
    {
        for (const DProperty* p = dclass->GetOwnProperties(); p; p = p->GetNext())
            properties.push_back(SerializePropertySchema(p));
    }

    nlohmann::json result;
    result["class_name"] = dclass->GetName();
    result["super"] = dclass->GetSuperName();
    result["is_abstract"] = dclass->IsAbstract();
    result["properties"] = std::move(properties);

    if (includeFunctions)
    {
        nlohmann::json functions = nlohmann::json::array();
        auto signatureKey = [](const DFunction* fn) -> std::string
        {
            std::string key = fn->GetName();
            key.push_back('(');
            const auto& params = fn->GetParams();
            for (size_t i = 0; i < params.size(); ++i)
            {
                if (i > 0)
                    key.push_back(',');
                if (params[i])
                    key.append(params[i]->GetType());
            }
            key.push_back(')');
            return key;
        };

        if (includeInherited)
        {
            std::unordered_set<std::string> seenNames;
            std::unordered_set<std::string> seenSignatures;
            for (const DStruct* s = dclass; s; s = s->GetSuper())
            {
                const DClass* cls = dynamic_cast<const DClass*>(s);
                if (!cls)
                    break;
                std::unordered_set<std::string> namesAddedAtThisLevel;
                for (DFunction* fn : cls->GetFunctions())
                {
                    if (!fn)
                        continue;
                    const std::string& fname = fn->GetName();
                    // Derived overload group shadows the base group with the same name.
                    if (seenNames.count(fname) && !namesAddedAtThisLevel.count(fname))
                        continue;
                    if (seenSignatures.insert(signatureKey(fn)).second)
                    {
                        functions.push_back(SerializeFunctionSchema(fn));
                        namesAddedAtThisLevel.insert(fname);
                    }
                }
                for (const auto& n : namesAddedAtThisLevel)
                    seenNames.insert(n);
            }
        }
        else
        {
            for (DFunction* fn : dclass->GetFunctions())
                functions.push_back(SerializeFunctionSchema(fn));
        }
        result["functions"] = std::move(functions);
    }

    return { {"ok", true}, {"schema", std::move(result)} };
}

// ─── inheritance_chain ──────────────────────────────────────────────────────

nlohmann::json McpReflectionSystem::QueryInheritanceChain(EditorCore&, const nlohmann::json& params)
{
    if (!params.contains("class_name"))
        return MakeError("missing required param: class_name");

    std::string className = params["class_name"].get<std::string>();
    DClass* dclass = GetReflectionRegistry().FindClassByName(className);
    if (!dclass)
        return MakeError("unknown class: " + className);

    nlohmann::json chain = nlohmann::json::array();
    for (const DStruct* s = dclass; s; s = s->GetSuper())
        chain.push_back(s->GetName());

    return { {"ok", true}, {"chain", std::move(chain)} };
}

// ─── find_classes_with_property ─────────────────────────────────────────────

nlohmann::json McpReflectionSystem::QueryFindClassesWithProperty(EditorCore&, const nlohmann::json& params)
{
    std::string propName = params.value("property_name", "");
    std::string propType = params.value("property_type", "");
    std::string matchMode = params.value("match_mode", "exact");

    if (propName.empty() && propType.empty())
        return MakeError("at least one of property_name or property_type is required");

    auto matches = [&](const std::string& haystack, const std::string& needle) -> bool
    {
        if (needle.empty())
            return true;
        if (matchMode == "contains")
            return haystack.find(needle) != std::string::npos;
        return haystack == needle;
    };

    nlohmann::json arr = nlohmann::json::array();
    for (auto& [className, dclass] : GetReflectionRegistry().GetAllClasses())
    {
        for (const DProperty* p = dclass->GetOwnProperties(); p; p = p->GetNext())
        {
            if (matches(p->GetName(), propName) && matches(p->GetType(), propType))
            {
                arr.push_back({
                    {"class_name", className},
                    {"property_name", p->GetName()},
                    {"property_type", p->GetType()}
                });
            }
        }
    }

    return { {"ok", true}, {"matches", std::move(arr)} };
}
