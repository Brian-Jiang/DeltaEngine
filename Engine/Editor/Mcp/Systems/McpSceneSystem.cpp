#include "McpSceneSystem.h"

#include "Editor/Animation/EditorAnimationManager.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommand_CreateComponent.h"
#include "Editor/Commands/EditorCommand_CreateGameObject.h"
#include "Editor/Commands/EditorCommand_DeleteComponent.h"
#include "Editor/Commands/EditorCommand_DeleteGameObject.h"
#include "Editor/Commands/EditorCommand_DuplicateGameObject.h"
#include "Editor/Commands/EditorCommand_RenameObject.h"
#include "Editor/Commands/EditorCommand_ReparentSceneComponent.h"
#include "Editor/Commands/EditorCommand_SetProperty.h"
#include "Editor/Commands/PropertyValueIO.h"
#include "Editor/EditorCore.h"
#include "Editor/Mcp/McpAnimationDefaults.h"
#include "Mcp/McpProtocol.h"
#include "Mcp/McpRegistry.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/DComponent.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Reflection/DEnumProperty.h"
#include "Runtime/Logging/LogCategory.h"

#include <array>
#include <memory>
#include <optional>
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

static nlohmann::json SerializeProperties(
    EditorCore& core,
    const DObject* obj,
    const DClass* dclass,
    const std::unordered_set<std::string>& includeFields)
{
    nlohmann::json props = nlohmann::json::object();
    for (const DProperty* p = dclass->GetProperties(); p; p = p->GetHierarchyNext())
    {
        if (!includeFields.empty() && !includeFields.contains(p->GetName()))
            continue;

        nlohmann::json val = PropertyToJson(obj, p, core);
        if (!val.is_null())
            props[p->GetName()] = std::move(val);
    }
    return props;
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
        if (go->GetObjectId() == target)
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
        if (go->GetObjectId() == target)
            return go;

        for (DComponent* comp : go->GetComponents())
        {
            if (comp->GetObjectId() == target)
                return comp;
        }
        for (SceneComponent* sc : go->GetSceneComponents())
        {
            if (sc->GetObjectId() == target)
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
    registry.RegisterQuery("scene", "game_objects",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryGameObjects(c, p); });
    registry.RegisterQuery("scene", "game_object",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryGameObject(c, p); });
    registry.RegisterQuery("scene", "hierarchy",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryHierarchy(c, p); });
    registry.RegisterQuery("scene", "component",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryComponent(c, p); });
    registry.RegisterQuery("scene", "components_on_object",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryComponentsOnObject(c, p); });
    registry.RegisterQuery("scene", "find_by_property",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryFindByProperty(c, p); });
    registry.RegisterQuery("scene", "get_position",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryGetPosition(c, p); });
    registry.RegisterQuery("scene", "get_rotation",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryGetRotation(c, p); });
    registry.RegisterQuery("scene", "get_scale",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryGetScale(c, p); });

    registry.RegisterCommand("scene", "CreateGameObject",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandCreateGameObject(c, p); });
    registry.RegisterCommand("scene", "DeleteGameObject",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandDeleteGameObject(c, p); });
    registry.RegisterCommand("scene", "DuplicateGameObject",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandDuplicateGameObject(c, p); });
    registry.RegisterCommand("scene", "ReparentSceneComponent",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandReparentSceneComponent(c, p); });
    registry.RegisterCommand("scene", "CreateComponent",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandCreateComponent(c, p); });
    registry.RegisterCommand("scene", "DeleteComponent",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandDeleteComponent(c, p); });
    registry.RegisterCommand("scene", "SetPosition",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandSetPosition(c, p); });
    registry.RegisterCommand("scene", "SetRotation",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandSetRotation(c, p); });
    registry.RegisterCommand("scene", "SetScale",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandSetScale(c, p); });
}

// ─── game_objects ───────────────────────────────────────────────────────────

nlohmann::json McpSceneSystem::QueryGameObjects(EditorCore& core, const nlohmann::json& params)
{
    DWorld* world = core.GetWorld();
    if (!world)
        return MakeMcpError("no active world");

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
            entry["properties"] = SerializeProperties(core, go, go->GetClass(), includeFields);

        arr.push_back(std::move(entry));
    }

    return { {"ok", true}, {"game_objects", std::move(arr)} };
}

// ─── game_object ────────────────────────────────────────────────────────────

nlohmann::json McpSceneSystem::QueryGameObject(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("object_id"))
        return MakeMcpError("missing required param: object_id");

    std::string objectIdStr = params["object_id"].get<std::string>();
    GameObject* go = FindGameObjectById(core, objectIdStr);
    if (!go)
        return MakeMcpError("GameObject not found: " + objectIdStr);

    auto includeFields = ParseIncludeFields(params);
    auto [assetId, objectId] = core.GetIdsForObject(go);

    nlohmann::json result;
    result["object_id"] = objectId.ToString();
    result["asset_id"] = assetId.ToString();
    result["name"] = go->GetName();
    result["class"] = go->GetClass() ? go->GetClass()->GetName() : "GameObject";

    if (go->GetClass())
        result["properties"] = SerializeProperties(core, go, go->GetClass(), includeFields);

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
        return MakeMcpError("no active world");

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
            return MakeMcpError("root object not found or has no SceneComponent: " + rootIdStr);
    }
    else
    {
        root = world->GetRootSceneComponent();
        if (!root)
            return MakeMcpError("world has no root SceneComponent");
    }

    return { {"ok", true}, {"hierarchy", BuildHierarchyNode(core, root)} };
}

// ─── component ──────────────────────────────────────────────────────────────

nlohmann::json McpSceneSystem::QueryComponent(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("object_id"))
        return MakeMcpError("missing required param: object_id");

    std::string objectIdStr = params["object_id"].get<std::string>();
    DObject* obj = FindObjectById(core, objectIdStr);
    if (!obj)
        return MakeMcpError("object not found: " + objectIdStr);

    DClass* dclass = obj->GetClass();
    if (!dclass)
        return MakeMcpError("object has no reflection class");

    auto [assetId, objectId] = core.GetIdsForObject(obj);
    //std::unordered_set<std::string> noFilter;

    auto includeFields = ParseIncludeFields(params);
    nlohmann::json result;
    result["object_id"] = objectId.ToString();
    result["class"] = dclass->GetName();
    result["properties"] = SerializeProperties(core, obj, dclass, includeFields);

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
        return MakeMcpError("missing required param: object_id");

    std::string objectIdStr = params["object_id"].get<std::string>();
    GameObject* go = FindGameObjectById(core, objectIdStr);
    if (!go)
        return MakeMcpError("GameObject not found: " + objectIdStr);

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
            entry["properties"] = SerializeProperties(core, comp, dc, noFilter);

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
        return MakeMcpError("missing required params: class_name, property_name, value");

    std::string className = params["class_name"].get<std::string>();
    std::string propertyName = params["property_name"].get<std::string>();
    const nlohmann::json& targetValue = params["value"];

    DClass* dclass = GetReflectionRegistry().FindClassByName(className);
    if (!dclass)
        return MakeMcpError("unknown class: " + className);

    DProperty* prop = dclass->FindPropertyByName(propertyName);
    if (!prop)
        return MakeMcpError("unknown property '" + propertyName + "' on class " + className);

    std::vector<DObject*> allObjects;
    CollectAllObjects(core, allObjects);

    nlohmann::json matches = nlohmann::json::array();

    for (DObject* obj : allObjects)
    {
        DClass* objClass = obj->GetClass();
        if (!objClass || !objClass->IsChildOf(dclass))
            continue;

        nlohmann::json propVal = PropertyToJson(obj, prop, core);
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

// ─── Command: CreateGameObject ──────────────────────────────────────────────

nlohmann::json McpSceneSystem::CommandCreateGameObject(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("name"))
        return MakeMcpError("missing required param: name");

    const std::string name = params["name"].get<std::string>();

    const AssetId sceneAssetId = ActiveSceneAssetId(core);
    if (sceneAssetId.IsNull())
        return MakeMcpError("No active scene asset");

    // Create the GameObject.
    nlohmann::json err;
    auto createCmd = std::make_unique<EditorCommand_CreateGameObject>(sceneAssetId, "GameObject");
    EditorCommand_CreateGameObject* createPtr = createCmd.get();
    if (!ExecuteMcpCommand(core, std::move(createCmd), err))
        return err;

    const ObjectId createdId = createPtr->GetCreatedObjectId();
    const std::string objectId = createdId.ToString();

    const bool needsRename = !name.empty() && name != "New GameObject";
    if (!needsRename)
    {
        auto res = MakeMcpOk();
        res["objectId"] = objectId;
        return res;
    }

    // Rename to the requested name as a second undoable step.
    if (!ExecuteMcpCommand(
            core, std::make_unique<EditorCommand_RenameObject>(sceneAssetId, createdId, name), err))
    {
        err["objectId"] = objectId;
        return err;
    }

    auto res = MakeMcpOk();
    res["objectId"] = objectId;
    return res;
}

// ─── Command: DeleteGameObject ──────────────────────────────────────────────

nlohmann::json McpSceneSystem::CommandDeleteGameObject(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("objectId"))
        return MakeMcpError("missing required param: objectId");

    const ObjectId gameObjectId = UUID::FromString(params["objectId"].get<std::string>());
    if (gameObjectId.IsNull())
        return MakeMcpError("invalid objectId");

    nlohmann::json err;
    if (!ExecuteMcpCommand(core, std::make_unique<EditorCommand_DeleteGameObject>(
            ActiveSceneAssetId(core), gameObjectId), err))
        return err;

    return MakeMcpOk();
}

// ─── Command: DuplicateGameObject ───────────────────────────────────────────

nlohmann::json McpSceneSystem::CommandDuplicateGameObject(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("objectId"))
        return MakeMcpError("missing required param: objectId");

    const AssetId assetId = ActiveSceneAssetId(core);
    const ObjectId sourceObjectId = UUID::FromString(params["objectId"].get<std::string>());

    std::string newName;
    if (params.contains("newName") && params["newName"].is_string())
        newName = params["newName"].get<std::string>();

    std::optional<std::array<float, 3>> offsetPosition;
    if (params.contains("offset_position") && params["offset_position"].is_array() &&
        params["offset_position"].size() >= 3)
    {
        const auto& off = params["offset_position"];
        offsetPosition = std::array<float, 3>{
            off[0].get<float>(), off[1].get<float>(), off[2].get<float>()};
    }

    auto cmd = std::make_unique<EditorCommand_DuplicateGameObject>(
        assetId, sourceObjectId, std::move(newName), offsetPosition);
    EditorCommand_DuplicateGameObject* cmdPtr = cmd.get();

    nlohmann::json err;
    if (!ExecuteMcpCommand(core, std::move(cmd), err))
        return err;

    auto res = MakeMcpOk();
    res["objectId"] = cmdPtr->GetCreatedObjectId().ToString();
    return res;
}

// ─── Command: ReparentSceneComponent ────────────────────────────────────────

nlohmann::json McpSceneSystem::CommandReparentSceneComponent(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("objectId"))
        return MakeMcpError("missing required param: objectId");
    if (!params.contains("newParentId"))
        return MakeMcpError("missing required param: newParentId");

    const ObjectId childObjectId = UUID::FromString(params["objectId"].get<std::string>());
    if (childObjectId.IsNull())
        return MakeMcpError("invalid objectId");
    const ObjectId newParentObjectId = UUID::FromString(params["newParentId"].get<std::string>());
    if (newParentObjectId.IsNull())
        return MakeMcpError("invalid newParentId");

    nlohmann::json err;
    if (!ExecuteMcpCommand(core, std::make_unique<EditorCommand_ReparentSceneComponent>(
            ActiveSceneAssetId(core), childObjectId, newParentObjectId), err))
        return err;

    return MakeMcpOk();
}

// ─── Command: CreateComponent ───────────────────────────────────────────────

nlohmann::json McpSceneSystem::CommandCreateComponent(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("objectId"))
        return MakeMcpError("missing required param: objectId");
    if (!params.contains("componentClass"))
        return MakeMcpError("missing required param: componentClass");

    const ObjectId gameObjectId = UUID::FromString(params["objectId"].get<std::string>());
    if (gameObjectId.IsNull())
        return MakeMcpError("invalid objectId");

    auto cmd = std::make_unique<EditorCommand_CreateComponent>(
        ActiveSceneAssetId(core), gameObjectId, params["componentClass"].get<std::string>());
    EditorCommand_CreateComponent* cmdPtr = cmd.get();

    nlohmann::json err;
    if (!ExecuteMcpCommand(core, std::move(cmd), err))
        return err;

    auto res = MakeMcpOk();
    res["objectId"] = cmdPtr->GetCreatedComponentId().ToString();
    return res;
}

// ─── Command: DeleteComponent ───────────────────────────────────────────────

nlohmann::json McpSceneSystem::CommandDeleteComponent(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("objectId"))
        return MakeMcpError("missing required param: objectId");

    std::string compIdStr = params["objectId"].get<std::string>();

    DObject* obj = FindObjectById(core, compIdStr);
    if (!obj)
        return MakeMcpError("component not found: " + compIdStr);

    auto* comp = dynamic_cast<DComponent*>(obj);
    if (!comp)
        return MakeMcpError("object is not a component: " + compIdStr);

    GameObject* owner = comp->GetGameObject();
    if (!owner)
        return MakeMcpError("component has no owning GameObject: " + compIdStr);

    const ObjectId ownerObjectId = owner->GetObjectId();
    const ObjectId componentId = comp->GetObjectId();

    nlohmann::json err;
    if (!ExecuteMcpCommand(core, std::make_unique<EditorCommand_DeleteComponent>(
            ActiveSceneAssetId(core), ownerObjectId, componentId), err))
        return err;

    return MakeMcpOk();
}

// ─── Shared helper: resolve a SceneComponent from an objectId string ────────

static SceneComponent* ResolveSceneComponent(EditorCore& core, const std::string& objectIdStr)
{
    DObject* obj = FindObjectById(core, objectIdStr);
    if (!obj)
        return nullptr;
    if (auto* sc = dynamic_cast<SceneComponent*>(obj))
        return sc;
    if (auto* go = dynamic_cast<GameObject*>(obj))
        return go->GetRootSceneComponent();
    return nullptr;
}

namespace
{
using namespace DirectX;
using namespace DirectX::SimpleMath;

constexpr const char* kLocalPositionProp = "m_localPosition";
constexpr const char* kLocalRotationProp = "m_localRotation";
constexpr const char* kLocalScaleProp    = "m_localScale";
constexpr const char* kLocalEulerProp    = "m_localEulerAngles";

// Decomposed property an animation channel commits. Mirrors PropertyNameForChannel in
// EditorAnimationManager.cpp — rotation commits the euler hint, the quaternion is rebuilt from it.
const char* ChannelPropertyName(const std::string& channel)
{
    if (channel == "position") return kLocalPositionProp;
    if (channel == "scale")    return kLocalScaleProp;
    if (channel == "rotation") return kLocalEulerProp;
    return nullptr;
}

nlohmann::json Vector3ToJson(const Vector3& v)
{
    return nlohmann::json::array({ v.x, v.y, v.z });
}

nlohmann::json QuaternionToJson(const Quaternion& q)
{
    return nlohmann::json::array({ q.x, q.y, q.z, q.w });
}

Vector3 EulerDegreesFromQuaternion(const Quaternion& q)
{
    const Vector3 radians = q.ToEuler();
    return Vector3(
        XMConvertToDegrees(radians.x),
        XMConvertToDegrees(radians.y),
        XMConvertToDegrees(radians.z));
}

// Matches SceneComponent::SetLocalRotation(Vector3) — degrees, roll/pitch/yaw order.
Quaternion QuaternionFromEulerDegrees(const Vector3& degrees)
{
    return Quaternion(XMQuaternionRotationRollPitchYaw(
        XMConvertToRadians(degrees.x),
        XMConvertToRadians(degrees.y),
        XMConvertToRadians(degrees.z)));
}

// Mirrors SceneComponent::SetWorldPosition — through the parent's full inverse world matrix.
Vector3 WorldToLocalPosition(const SceneComponent* sc, const Vector3& worldPosition)
{
    const SceneComponent* parent = sc->GetParent();
    if (!parent)
        return worldPosition;

    const XMMATRIX invParent = XMMatrixInverse(nullptr, parent->GetWorldTransform());
    return Vector3(XMVector3TransformCoord(
        XMVectorSet(worldPosition.x, worldPosition.y, worldPosition.z, 1.0f), invParent));
}

// Mirrors SceneComponent::SetWorldRotation.
Quaternion WorldToLocalRotation(const SceneComponent* sc, const Quaternion& worldRotation)
{
    const SceneComponent* parent = sc->GetParent();
    if (!parent)
        return worldRotation;

    const XMVECTOR parentInverse = XMQuaternionInverse((XMVECTOR) parent->GetWorldRotation());
    return Quaternion(XMQuaternionMultiply((XMVECTOR) worldRotation, parentInverse));
}

// Rotation wire format: 4 elements = quaternion [x,y,z,w], 3 elements = euler angles in degrees.
bool ParseRotationValue(const nlohmann::json& value, Quaternion& outRotation)
{
    if (value.size() == 4)
    {
        outRotation = Quaternion(value[0].get<float>(), value[1].get<float>(),
                                 value[2].get<float>(), value[3].get<float>());
        return true;
    }
    if (value.size() == 3)
    {
        outRotation = QuaternionFromEulerDegrees(
            Vector3(value[0].get<float>(), value[1].get<float>(), value[2].get<float>()));
        return true;
    }
    return false;
}

bool ParseSpace(const nlohmann::json& params, bool& outWorldSpace, nlohmann::json& outError)
{
    const std::string space = params.value("space", "local");
    if (space == "local")
    {
        outWorldSpace = false;
        return true;
    }
    if (space == "world")
    {
        outWorldSpace = true;
        return true;
    }
    outError = MakeMcpError("'space' must be \"local\" or \"world\", got: " + space);
    return false;
}
} // namespace

// Starts (or, in headless mode, immediately applies + commits) a tweened transform-channel
// animation for one of "position" / "rotation" / "scale". The animation manager commits one
// SetProperty per participating decomposed property once every channel of the session finishes;
// headless has no manager, so it applies the setter and commits this channel's property now.
static nlohmann::json StartTransformChannelAnimation(
    EditorCore& core, SceneComponent* sc,
    const AssetId& scAssetId, const ObjectId& scObjectId,
    const std::string& channel, const nlohmann::json& tval,
    bool worldSpace, float duration)
{
    using namespace DirectX::SimpleMath;

    const char* channelPropName = ChannelPropertyName(channel);
    if (!channelPropName)
        return MakeMcpError("Unknown channel: " + channel);

    DProperty* channelProp = sc->GetClass()->FindPropertyByName(channelPropName);
    if (!channelProp)
        return MakeMcpError(std::string(channelPropName) + " property not found");

    const nlohmann::json snapshot = PropertyToJson(sc, channelProp);
    EditorAnimationManager* animMgr = core.GetAnimationManager();

    auto commitHeadless = [&]()
    {
        EditorCommandContext ctx{core};
        core.GetCommandManager().Execute(
            std::make_unique<EditorCommand_SetProperty>(
                scAssetId, scObjectId, channelPropName,
                snapshot, PropertyToJson(sc, channelProp)),
            ctx);
    };

    if (channel == "position")
    {
        const Vector3 from = worldSpace ? sc->GetWorldPosition() : sc->GetLocalPosition();
        const Vector3 target(tval[0].get<float>(), tval[1].get<float>(), tval[2].get<float>());
        if (animMgr)
        {
            animMgr->StartAnimationVec3(
                scAssetId, scObjectId, "position", from, target, duration,
                worldSpace
                    ? std::function<void(Vector3)>([sc](Vector3 p) { sc->SetWorldPosition(p); })
                    : std::function<void(Vector3)>([sc](Vector3 p) { sc->SetLocalPosition(p); }),
                snapshot);
        }
        else
        {
            if (worldSpace) sc->SetWorldPosition(target); else sc->SetLocalPosition(target);
            commitHeadless();
        }
    }
    else if (channel == "rotation")
    {
        const Quaternion from = worldSpace ? sc->GetWorldRotation() : sc->GetLocalRotation();
        Quaternion target;
        if (!ParseRotationValue(tval, target))
            return MakeMcpError("'value' must have 3 (euler degrees) or 4 (quaternion) elements");

        if (animMgr)
        {
            animMgr->StartAnimationQuat(
                scAssetId, scObjectId, "rotation", from, target, duration,
                worldSpace
                    ? std::function<void(Quaternion)>([sc](Quaternion q) { sc->SetWorldRotation(q); })
                    : std::function<void(Quaternion)>([sc](Quaternion q) { sc->SetLocalRotation(q); }),
                snapshot);
        }
        else
        {
            if (worldSpace) sc->SetWorldRotation(target); else sc->SetLocalRotation(target);
            commitHeadless();
        }
    }
    else if (channel == "scale")
    {
        const Vector3 from = sc->GetLocalScale();
        const Vector3 target(tval[0].get<float>(), tval[1].get<float>(), tval[2].get<float>());
        if (animMgr)
        {
            animMgr->StartAnimationVec3(
                scAssetId, scObjectId, "scale", from, target, duration,
                [sc](Vector3 s) { sc->SetLocalScale(s); },
                snapshot);
        }
        else
        {
            sc->SetLocalScale(target);
            commitHeadless();
        }
    }

    return {{"ok", true}, {"commandType", "StartTransformChannelAnimation"}};
}

// ─── Command: SetPosition ────────────────────────────────────────────────────

nlohmann::json McpSceneSystem::CommandSetPosition(EditorCore& core, const nlohmann::json& params)
{
    using namespace DirectX::SimpleMath;

    if (!params.contains("objectId"))
        return MakeMcpError("missing required param: objectId");
    if (!params.contains("value") || !params["value"].is_array() || params["value"].size() < 3)
        return MakeMcpError("required param 'value' must be [x,y,z]");

    SceneComponent* sc = ResolveSceneComponent(core, params["objectId"].get<std::string>());
    if (!sc)
        return MakeMcpError("object has no SceneComponent: " + params["objectId"].get<std::string>());

    bool worldSpace = false;
    nlohmann::json err;
    if (!ParseSpace(params, worldSpace, err))
        return err;

    const auto& v = params["value"];
    const Vector3 target(v[0].get<float>(), v[1].get<float>(), v[2].get<float>());
    const float duration = params.value("duration_seconds", kDefaultAnimationDurationSeconds);

    auto [scAssetId, scObjectId] = core.GetIdsForObject(sc);

    if (duration <= 0.0f)
    {
        const Vector3 localPosition = worldSpace ? WorldToLocalPosition(sc, target) : target;
        if (!ExecuteMcpCommand(core, std::make_unique<EditorCommand_SetProperty>(
                scAssetId, scObjectId, kLocalPositionProp,
                nlohmann::json{}, Vector3ToJson(localPosition)), err))
            return err;
        return MakeMcpOk();
    }

    // Animation path.
    return StartTransformChannelAnimation(
        core, sc, scAssetId, scObjectId, "position", params["value"], worldSpace, duration);
}

// ─── Command: SetRotation ────────────────────────────────────────────────────

nlohmann::json McpSceneSystem::CommandSetRotation(EditorCore& core, const nlohmann::json& params)
{
    using namespace DirectX::SimpleMath;

    if (!params.contains("objectId"))
        return MakeMcpError("missing required param: objectId");
    if (!params.contains("value") || !params["value"].is_array())
        return MakeMcpError("required param 'value' must be [x,y,z,w] or [x,y,z] (euler degrees)");

    SceneComponent* sc = ResolveSceneComponent(core, params["objectId"].get<std::string>());
    if (!sc)
        return MakeMcpError("object has no SceneComponent: " + params["objectId"].get<std::string>());

    const auto& vArr = params["value"];
    Quaternion target;
    if (!ParseRotationValue(vArr, target))
        return MakeMcpError("'value' must have 3 (euler degrees) or 4 (quaternion) elements");

    bool worldSpace = false;
    nlohmann::json err;
    if (!ParseSpace(params, worldSpace, err))
        return err;

    const float duration = params.value("duration_seconds", kDefaultAnimationDurationSeconds);

    auto [scAssetId, scObjectId] = core.GetIdsForObject(sc);

    if (duration <= 0.0f)
    {
        // Local euler input writes the euler property verbatim so the caller's angles survive
        // exactly (370 stays 370); PostEditChangeProperty rebuilds the quaternion from it.
        if (!worldSpace && vArr.size() == 3)
        {
            const Vector3 euler(vArr[0].get<float>(), vArr[1].get<float>(), vArr[2].get<float>());
            if (!ExecuteMcpCommand(core, std::make_unique<EditorCommand_SetProperty>(
                    scAssetId, scObjectId, kLocalEulerProp,
                    nlohmann::json{}, Vector3ToJson(euler)), err))
                return err;
            return MakeMcpOk();
        }

        const Quaternion localRotation = worldSpace ? WorldToLocalRotation(sc, target) : target;
        if (!ExecuteMcpCommand(core, std::make_unique<EditorCommand_SetProperty>(
                scAssetId, scObjectId, kLocalRotationProp,
                nlohmann::json{}, QuaternionToJson(localRotation)), err))
            return err;
        return MakeMcpOk();
    }

    return StartTransformChannelAnimation(
        core, sc, scAssetId, scObjectId, "rotation", params["value"], worldSpace, duration);
}

// ─── Command: SetScale ───────────────────────────────────────────────────────

nlohmann::json McpSceneSystem::CommandSetScale(EditorCore& core, const nlohmann::json& params)
{
    using namespace DirectX::SimpleMath;

    if (!params.contains("objectId"))
        return MakeMcpError("missing required param: objectId");
    if (!params.contains("value") || !params["value"].is_array() || params["value"].size() < 3)
        return MakeMcpError("required param 'value' must be [x,y,z]");
    // SceneComponent exposes GetWorldScale but has no SetWorldScale — local space only.
    if (params.value("space", "local") != "local")
        return MakeMcpError("SetScale supports local space only");

    SceneComponent* sc = ResolveSceneComponent(core, params["objectId"].get<std::string>());
    if (!sc)
        return MakeMcpError("object has no SceneComponent: " + params["objectId"].get<std::string>());

    const auto& v = params["value"];
    const Vector3 target(v[0].get<float>(), v[1].get<float>(), v[2].get<float>());
    const float duration = params.value("duration_seconds", kDefaultAnimationDurationSeconds);

    auto [scAssetId, scObjectId] = core.GetIdsForObject(sc);

    if (duration <= 0.0f)
    {
        nlohmann::json err;
        if (!ExecuteMcpCommand(core, std::make_unique<EditorCommand_SetProperty>(
                scAssetId, scObjectId, kLocalScaleProp,
                nlohmann::json{}, Vector3ToJson(target)), err))
            return err;
        return MakeMcpOk();
    }

    return StartTransformChannelAnimation(
        core, sc, scAssetId, scObjectId, "scale", params["value"], /*worldSpace*/ false, duration);
}

// ─── Queries: get_position / get_rotation / get_scale ───────────────────────

// Resolves the target SceneComponent and its own object id. Callers parse 'space' themselves
// so the schema/handler param sync check can see the read in the handler body.
static bool ResolveTransformQuery(
    EditorCore& core, const nlohmann::json& params,
    SceneComponent*& outComponent, std::string& outObjectId, nlohmann::json& outError)
{
    if (!params.contains("object_id"))
    {
        outError = MakeMcpError("missing required param: object_id");
        return false;
    }

    const std::string requestedId = params["object_id"].get<std::string>();
    outComponent = ResolveSceneComponent(core, requestedId);
    if (!outComponent)
    {
        outError = MakeMcpError("object has no SceneComponent: " + requestedId);
        return false;
    }

    // A GameObject id resolves to its root SceneComponent — echo the component actually read.
    auto [assetId, objectId] = core.GetIdsForObject(outComponent);
    outObjectId = objectId.ToString();

    return true;
}

nlohmann::json McpSceneSystem::QueryGetPosition(EditorCore& core, const nlohmann::json& params)
{
    SceneComponent* sc = nullptr;
    bool worldSpace = false;
    std::string objectId;
    nlohmann::json err;
    if (!ResolveTransformQuery(core, params, sc, objectId, err))
        return err;
    if (!ParseSpace(params, worldSpace, err))
        return err;

    nlohmann::json result = MakeMcpOk();
    result["object_id"] = objectId;
    result["space"] = worldSpace ? "world" : "local";
    result["position"] = Vector3ToJson(
        worldSpace ? sc->GetWorldPosition() : sc->GetLocalPosition());
    return result;
}

nlohmann::json McpSceneSystem::QueryGetRotation(EditorCore& core, const nlohmann::json& params)
{
    using namespace DirectX::SimpleMath;

    SceneComponent* sc = nullptr;
    bool worldSpace = false;
    std::string objectId;
    nlohmann::json err;
    if (!ResolveTransformQuery(core, params, sc, objectId, err))
        return err;
    if (!ParseSpace(params, worldSpace, err))
        return err;

    const Quaternion rotation = worldSpace ? sc->GetWorldRotation() : sc->GetLocalRotation();

    nlohmann::json result = MakeMcpOk();
    result["object_id"] = objectId;
    result["space"] = worldSpace ? "world" : "local";
    result["quaternion"] = QuaternionToJson(rotation);
    // Local euler is the stored user-facing hint (may exceed ±180); world is derived.
    result["euler"] = Vector3ToJson(worldSpace
        ? EulerDegreesFromQuaternion(rotation)
        : sc->GetLocalRotationEulerAngles());
    return result;
}

nlohmann::json McpSceneSystem::QueryGetScale(EditorCore& core, const nlohmann::json& params)
{
    SceneComponent* sc = nullptr;
    bool worldSpace = false;
    std::string objectId;
    nlohmann::json err;
    if (!ResolveTransformQuery(core, params, sc, objectId, err))
        return err;
    if (!ParseSpace(params, worldSpace, err))
        return err;

    nlohmann::json result = MakeMcpOk();
    result["object_id"] = objectId;
    result["space"] = worldSpace ? "world" : "local";
    result["scale"] = Vector3ToJson(worldSpace ? sc->GetWorldScale() : sc->GetLocalScale());
    return result;
}
