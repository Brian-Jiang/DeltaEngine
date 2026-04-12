#include "McpSceneSystem.h"

#include "Editor/EditorCore.h"
#include "Editor/Commands/PropertyValueIO.h"
#include "Mcp/McpRegistry.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/DComponent.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Logging/LogCategory.h"

#include <unordered_set>

using namespace DeltaEngine;

DEFINE_LOG_CATEGORY_STATIC(LogMcpScene);

static const char* PropertyTypeName(EPropertyType t)
{
    switch (t)
    {
    case EPropertyType::Float:      return "float";
    case EPropertyType::Int:        return "int";
    case EPropertyType::Bool:       return "bool";
    case EPropertyType::Double:     return "double";
    case EPropertyType::String:     return "string";
    case EPropertyType::WString:    return "wstring";
    case EPropertyType::Vector3:    return "Vector3";
    case EPropertyType::Quaternion: return "Quaternion";
    case EPropertyType::Float4:     return "Float4";
    case EPropertyType::Float4x4:   return "Float4x4";
    case EPropertyType::ObjectPtr:  return "ObjectPtr";
    case EPropertyType::BulkData:   return "BulkData";
    case EPropertyType::Vector:     return "Vector";
    case EPropertyType::Struct:     return "Struct";
    }
    return "unknown";
}

static nlohmann::json MakeError(const std::string& msg)
{
    return { {"ok", false}, {"error", msg} };
}

static nlohmann::json SerializeProperties(
    const DObject* obj,
    const DClass* dclass,
    const std::unordered_set<std::string>& includeFields)
{
    nlohmann::json props = nlohmann::json::object();
    for (const DProperty* p = dclass->GetProperties(); p; p = p->GetHierarchyNext())
    {
        if (!includeFields.empty() && !includeFields.contains(p->GetName()))
            continue;

        nlohmann::json val = PropertyToJson(obj, p);
        if (!val.is_null())
            props[p->GetName()] = std::move(val);
    }
    return props;
}

static nlohmann::json SerializePropertySchema(const DProperty* p)
{
    return {
        {"name", p->GetName()},
        {"cpp_type", p->GetType()},
        {"type", PropertyTypeName(p->GetPropertyType())}
    };
}

static std::unordered_set<std::string> ParseIncludeFields(const nlohmann::json& params)
{
    std::unordered_set<std::string> out;
    if (params.contains("include_fields") && params["include_fields"].is_array())
        for (auto& f : params["include_fields"])
            if (f.is_string())
                out.insert(f.get<std::string>());
    return out;
}

static GameObject* FindGameObjectById(EditorCore& core, const std::string& objectIdStr)
{
    DWorld* world = core.GetWorld();
    if (!world)
        return nullptr;

    ObjectId target = DeltaEngine::UUID::FromString(objectIdStr);
    if (target.IsNull())
        return nullptr;

    for (GameObject* go : world->GetGameObjects())
    {
        auto [assetId, objId] = core.GetIdsForObject(go);
        if (objId == target)
            return go;
    }
    return nullptr;
}

static DObject* FindObjectById(EditorCore& core, const std::string& objectIdStr)
{
    DWorld* world = core.GetWorld();
    if (!world)
        return nullptr;

    ObjectId target = DeltaEngine::UUID::FromString(objectIdStr);
    if (target.IsNull())
        return nullptr;

    for (GameObject* go : world->GetGameObjects())
    {
        auto [aId, oId] = core.GetIdsForObject(go);
        if (oId == target)
            return go;

        for (DComponent* comp : go->GetComponents())
        {
            auto [ca, co] = core.GetIdsForObject(comp);
            if (co == target)
                return comp;
        }
        for (SceneComponent* sc : go->GetSceneComponents())
        {
            auto [sa, so] = core.GetIdsForObject(sc);
            if (so == target)
                return sc;
        }
    }
    return nullptr;
}

static void CollectAllObjects(EditorCore& core, std::vector<DObject*>& out)
{
    DWorld* world = core.GetWorld();
    if (!world)
        return;

    for (GameObject* go : world->GetGameObjects())
    {
        out.push_back(go);
        for (DComponent* comp : go->GetComponents())
            out.push_back(comp);
        for (SceneComponent* sc : go->GetSceneComponents())
            out.push_back(sc);
    }
}

// ─── Registration ───────────────────────────────────────────────────────────

void McpSceneSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterOperation("scene", "game_objects",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryGameObjects(c, p); });
    registry.RegisterOperation("scene", "game_object",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryGameObject(c, p); });
    registry.RegisterOperation("scene", "hierarchy",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryHierarchy(c, p); });
    registry.RegisterOperation("scene", "component",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryComponent(c, p); });
    registry.RegisterOperation("scene", "components_on_object",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryComponentsOnObject(c, p); });
    registry.RegisterOperation("scene", "find_by_property",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryFindByProperty(c, p); });
}

// ─── game_objects ───────────────────────────────────────────────────────────

nlohmann::json McpSceneSystem::QueryGameObjects(EditorCore& core, const nlohmann::json& params)
{
    DWorld* world = core.GetWorld();
    if (!world)
        return MakeError("no active world");

    auto includeFields = ParseIncludeFields(params);
    std::string nameFilter = params.value("name_filter", "");
    std::string classFilter = params.value("class_filter", "");

    nlohmann::json arr = nlohmann::json::array();

    for (GameObject* go : world->GetGameObjects())
    {
        if (!nameFilter.empty() && go->GetName().find(nameFilter) == std::string::npos)
            continue;

        if (!classFilter.empty())
        {
            bool hasMatch = false;
            for (DComponent* comp : go->GetComponents())
                if (comp->GetClass() && comp->GetClass()->GetName() == classFilter)
                { hasMatch = true; break; }
            if (!hasMatch)
                for (SceneComponent* sc : go->GetSceneComponents())
                    if (sc->GetClass() && sc->GetClass()->GetName() == classFilter)
                    { hasMatch = true; break; }
            if (!hasMatch)
                continue;
        }

        auto [assetId, objectId] = core.GetIdsForObject(go);

        nlohmann::json entry;
        entry["object_id"] = objectId.ToString();
        entry["name"] = go->GetName();
        entry["class"] = go->GetClass() ? go->GetClass()->GetName() : "GameObject";

        if (go->GetClass())
            entry["properties"] = SerializeProperties(go, go->GetClass(), includeFields);

        arr.push_back(std::move(entry));
    }

    return { {"ok", true}, {"game_objects", std::move(arr)} };
}

// ─── game_object ────────────────────────────────────────────────────────────

nlohmann::json McpSceneSystem::QueryGameObject(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("object_id"))
        return MakeError("missing required param: object_id");

    std::string objectIdStr = params["object_id"].get<std::string>();
    GameObject* go = FindGameObjectById(core, objectIdStr);
    if (!go)
        return MakeError("GameObject not found: " + objectIdStr);

    auto includeFields = ParseIncludeFields(params);
    auto [assetId, objectId] = core.GetIdsForObject(go);

    nlohmann::json result;
    result["object_id"] = objectId.ToString();
    result["asset_id"] = assetId.ToString();
    result["name"] = go->GetName();
    result["class"] = go->GetClass() ? go->GetClass()->GetName() : "GameObject";

    if (go->GetClass())
        result["properties"] = SerializeProperties(go, go->GetClass(), includeFields);

    nlohmann::json comps = nlohmann::json::array();
    for (DComponent* comp : go->GetComponents())
    {
        auto [ca, co] = core.GetIdsForObject(comp);
        comps.push_back({
            {"object_id", co.ToString()},
            {"class", comp->GetClass() ? comp->GetClass()->GetName() : ""},
            {"name", comp->GetName()}
        });
    }
    for (SceneComponent* sc : go->GetSceneComponents())
    {
        auto [sa, so] = core.GetIdsForObject(sc);
        comps.push_back({
            {"object_id", so.ToString()},
            {"class", sc->GetClass() ? sc->GetClass()->GetName() : ""},
            {"name", sc->GetName()}
        });
    }
    result["components"] = std::move(comps);

    return { {"ok", true}, {"game_object", std::move(result)} };
}

// ─── hierarchy ──────────────────────────────────────────────────────────────

static nlohmann::json BuildHierarchyNode(EditorCore& core, SceneComponent* sc)
{
    auto [aId, oId] = core.GetIdsForObject(sc);

    nlohmann::json node;
    node["object_id"] = oId.ToString();
    node["name"] = sc->GetName();
    node["class"] = sc->GetClass() ? sc->GetClass()->GetName() : "";

    GameObject* owner = sc->GetGameObject();
    if (owner)
        node["game_object_name"] = owner->GetName();

    nlohmann::json children = nlohmann::json::array();
    for (SceneComponent* child : sc->GetChildren())
        children.push_back(BuildHierarchyNode(core, child));
    node["children"] = std::move(children);

    return node;
}

nlohmann::json McpSceneSystem::QueryHierarchy(EditorCore& core, const nlohmann::json& params)
{
    DWorld* world = core.GetWorld();
    if (!world)
        return MakeError("no active world");

    SceneComponent* root = nullptr;

    if (params.contains("root_object_id"))
    {
        std::string rootIdStr = params["root_object_id"].get<std::string>();
        DObject* obj = FindObjectById(core, rootIdStr);

        if (auto* go = dynamic_cast<GameObject*>(obj))
            root = go->GetRootSceneComponent();
        else if (auto* sc = dynamic_cast<SceneComponent*>(obj))
            root = sc;

        if (!root)
            return MakeError("root object not found or has no SceneComponent: " + rootIdStr);
    }
    else
    {
        root = world->GetRootSceneComponent();
        if (!root)
            return MakeError("world has no root SceneComponent");
    }

    return { {"ok", true}, {"hierarchy", BuildHierarchyNode(core, root)} };
}

// ─── component ──────────────────────────────────────────────────────────────

nlohmann::json McpSceneSystem::QueryComponent(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("object_id"))
        return MakeError("missing required param: object_id");

    std::string objectIdStr = params["object_id"].get<std::string>();
    DObject* obj = FindObjectById(core, objectIdStr);
    if (!obj)
        return MakeError("object not found: " + objectIdStr);

    DClass* dclass = obj->GetClass();
    if (!dclass)
        return MakeError("object has no reflection class");

    auto [assetId, objectId] = core.GetIdsForObject(obj);
    //std::unordered_set<std::string> noFilter;

    auto includeFields = ParseIncludeFields(params);
    nlohmann::json result;
    result["object_id"] = objectId.ToString();
    result["class"] = dclass->GetName();
    result["properties"] = SerializeProperties(obj, dclass, includeFields);

    if (auto* comp = dynamic_cast<DComponent*>(obj))
    {
        result["name"] = comp->GetName();
        if (GameObject* owner = comp->GetGameObject())
        {
            auto [ownerAssetId, ownerObjectId] = core.GetIdsForObject(owner);
            result["game_object_id"] = ownerObjectId.ToString();
            result["game_object_name"] = owner->GetName();
        }
    }

    bool includeSchema = params.value("include_schema", false);
    if (includeSchema)
    {
        nlohmann::json schema = nlohmann::json::array();
        for (const DProperty* p = dclass->GetProperties(); p; p = p->GetHierarchyNext())
            schema.push_back(SerializePropertySchema(p));
        result["schema"] = std::move(schema);
    }

    return { {"ok", true}, {"component", std::move(result)} };
}

// ─── components_on_object ───────────────────────────────────────────────────

nlohmann::json McpSceneSystem::QueryComponentsOnObject(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("object_id"))
        return MakeError("missing required param: object_id");

    std::string objectIdStr = params["object_id"].get<std::string>();
    GameObject* go = FindGameObjectById(core, objectIdStr);
    if (!go)
        return MakeError("GameObject not found: " + objectIdStr);

    bool includeProperties = params.value("include_properties", false);
    std::unordered_set<std::string> noFilter;

    nlohmann::json arr = nlohmann::json::array();

    auto serializeComp = [&](DObject* comp, const std::string& name)
    {
        auto [aId, oId] = core.GetIdsForObject(comp);
        DClass* dc = comp->GetClass();

        nlohmann::json entry;
        entry["object_id"] = oId.ToString();
        entry["class"] = dc ? dc->GetName() : "";
        entry["name"] = name;

        if (includeProperties && dc)
            entry["properties"] = SerializeProperties(comp, dc, noFilter);

        arr.push_back(std::move(entry));
    };

    for (DComponent* comp : go->GetComponents())
        serializeComp(comp, comp->GetName());
    for (SceneComponent* sc : go->GetSceneComponents())
        serializeComp(sc, sc->GetName());

    return { {"ok", true}, {"components", std::move(arr)} };
}

// ─── find_by_property ───────────────────────────────────────────────────────

nlohmann::json McpSceneSystem::QueryFindByProperty(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("class_name") || !params.contains("property_name") || !params.contains("value"))
        return MakeError("missing required params: class_name, property_name, value");

    std::string className = params["class_name"].get<std::string>();
    std::string propertyName = params["property_name"].get<std::string>();
    const nlohmann::json& targetValue = params["value"];

    DClass* dclass = GetReflectionRegistry().FindClassByName(className);
    if (!dclass)
        return MakeError("unknown class: " + className);

    DProperty* prop = dclass->FindPropertyByName(propertyName);
    if (!prop)
        return MakeError("unknown property '" + propertyName + "' on class " + className);

    std::vector<DObject*> allObjects;
    CollectAllObjects(core, allObjects);

    nlohmann::json matches = nlohmann::json::array();

    for (DObject* obj : allObjects)
    {
        DClass* objClass = obj->GetClass();
        if (!objClass || !objClass->IsChildOf(dclass))
            continue;

        nlohmann::json propVal = PropertyToJson(obj, prop);
        if (propVal == targetValue)
        {
            auto [aId, oId] = core.GetIdsForObject(obj);
            matches.push_back({
                {"object_id", oId.ToString()},
                {"class", objClass->GetName()},
                {"value", propVal}
            });
        }
    }

    return { {"ok", true}, {"matches", std::move(matches)} };
}
