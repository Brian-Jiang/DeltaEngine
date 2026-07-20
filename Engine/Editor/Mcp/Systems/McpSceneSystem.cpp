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

#include <memory>
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

    // DuplicateGameObject carries an optional position offset that is only settable
    // through Deserialize (no constructor arg), so populate it that way.
    nlohmann::json data;
    data["assetId"] = ActiveSceneAssetId(core).ToString();
    data["sourceObjectId"] = params["objectId"].get<std::string>();
    if (params.contains("newName") && params["newName"].is_string())
        data["newName"] = params["newName"].get<std::string>();
    if (params.contains("offset_position") && params["offset_position"].is_array())
        data["offsetPosition"] = params["offset_position"];

    auto cmd = std::make_unique<EditorCommand_DuplicateGameObject>();
    cmd->Deserialize(data);
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

// Serialise a local-space XMMATRIX to a 16-element JSON array.
static nlohmann::json MatrixToJson(DirectX::XMMATRIX mat)
{
    DirectX::XMFLOAT4X4 f;
    DirectX::XMStoreFloat4x4(&f, mat);
    nlohmann::json arr = nlohmann::json::array();
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            arr.push_back(f.m[r][c]);
    return arr;
}

// Starts (or, in headless mode, immediately applies + commits) a tweened transform-channel
// animation for one of "position" / "rotation" / "scale". On completion the animation manager
// commits a single m_localTransform SetProperty; headless applies the setter then commits now.
static nlohmann::json StartTransformChannelAnimation(
    EditorCore& core, SceneComponent* sc,
    const AssetId& scAssetId, const ObjectId& scObjectId,
    const std::string& channel, const nlohmann::json& tval,
    bool worldSpace, float duration)
{
    using namespace DirectX;
    using namespace DirectX::SimpleMath;

    DProperty* transformProp = sc->GetClass()->FindPropertyByName("m_localTransform");
    if (!transformProp)
        return MakeMcpError("m_localTransform property not found");

    const nlohmann::json snapshot = PropertyToJson(sc, transformProp);
    EditorAnimationManager* animMgr = core.GetAnimationManager();
    EditorCommandContext ctx{core};

    auto commitHeadless = [&]()
    {
        core.GetCommandManager().Execute(
            std::make_unique<EditorCommand_SetProperty>(
                scAssetId, scObjectId, "m_localTransform",
                snapshot, PropertyToJson(sc, transformProp)),
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
        if (tval.size() == 4)
            target = Quaternion(tval[0].get<float>(), tval[1].get<float>(),
                                tval[2].get<float>(), tval[3].get<float>());
        else
            target = Quaternion::CreateFromYawPitchRoll(
                tval[1].get<float>(), tval[0].get<float>(), tval[2].get<float>());

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
    else
    {
        return MakeMcpError("Unknown channel: " + channel);
    }

    return {{"ok", true}, {"commandType", "StartTransformChannelAnimation"}};
}

// ─── Command: SetPosition ────────────────────────────────────────────────────

nlohmann::json McpSceneSystem::CommandSetPosition(EditorCore& core, const nlohmann::json& params)
{
    using namespace DirectX;
    using namespace DirectX::SimpleMath;

    if (!params.contains("objectId"))
        return MakeMcpError("missing required param: objectId");
    if (!params.contains("value") || !params["value"].is_array() || params["value"].size() < 3)
        return MakeMcpError("required param 'value' must be [x,y,z]");

    SceneComponent* sc = ResolveSceneComponent(core, params["objectId"].get<std::string>());
    if (!sc)
        return MakeMcpError("object has no SceneComponent: " + params["objectId"].get<std::string>());

    const auto& v     = params["value"];
    const Vector3 target(v[0].get<float>(), v[1].get<float>(), v[2].get<float>());
    const std::string space = params.value("space", "local");
    const bool worldSpace   = (space == "world");
    //const float duration    = params.value("duration_seconds", kDefaultAnimationDurationSeconds);
    const float duration = kDefaultAnimationDurationSeconds;

    auto [scAssetId, scObjectId] = core.GetIdsForObject(sc);

    if (duration <= 0.0f)
    {
        // Immediate: reconstruct local transform with new position, then enqueue SetProperty.
        const Quaternion rot = worldSpace ? sc->GetWorldRotation() : sc->GetLocalRotation();
        const Vector3    scl = worldSpace ? sc->GetWorldScale()    : sc->GetLocalScale();

        XMMATRIX localMat;
        if (worldSpace)
        {
            const XMMATRIX world =
                XMMatrixScalingFromVector(scl) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(target);
            const SceneComponent* parent = sc->GetParent();
            const XMMATRIX parentWorldInv = parent
                ? XMMatrixInverse(nullptr, parent->GetWorldTransform())
                : XMMatrixIdentity();
            localMat = world * parentWorldInv;
        }
        else
        {
            localMat =
                XMMatrixScalingFromVector(scl) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(target);
        }

        nlohmann::json err;
        if (!ExecuteMcpCommand(core, std::make_unique<EditorCommand_SetProperty>(
                scAssetId, scObjectId, "m_localTransform", nlohmann::json{}, MatrixToJson(localMat)), err))
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
    using namespace DirectX;
    using namespace DirectX::SimpleMath;

    if (!params.contains("objectId"))
        return MakeMcpError("missing required param: objectId");
    if (!params.contains("value") || !params["value"].is_array())
        return MakeMcpError("required param 'value' must be [x,y,z,w] or [x,y,z] (euler)");

    SceneComponent* sc = ResolveSceneComponent(core, params["objectId"].get<std::string>());
    if (!sc)
        return MakeMcpError("object has no SceneComponent: " + params["objectId"].get<std::string>());

    const auto& vArr = params["value"];
    Quaternion target;
    if (vArr.size() == 4)
        target = Quaternion(vArr[0].get<float>(), vArr[1].get<float>(), vArr[2].get<float>(), vArr[3].get<float>());
    else if (vArr.size() == 3)
        target = Quaternion::CreateFromYawPitchRoll(vArr[1].get<float>(), vArr[0].get<float>(), vArr[2].get<float>());
    else
        return MakeMcpError("'value' must have 3 or 4 elements");

    const std::string space = params.value("space", "local");
    const bool worldSpace   = (space == "world");
    //const float duration    = params.value("duration_seconds", kDefaultAnimationDurationSeconds);
    const float duration = kDefaultAnimationDurationSeconds;

    auto [scAssetId, scObjectId] = core.GetIdsForObject(sc);

    if (duration <= 0.0f)
    {
        const Vector3 pos = worldSpace ? sc->GetWorldPosition() : sc->GetLocalPosition();
        const Vector3 scl = worldSpace ? sc->GetWorldScale()    : sc->GetLocalScale();

        XMMATRIX localMat;
        if (worldSpace)
        {
            const XMMATRIX world =
                XMMatrixScalingFromVector(scl) * XMMatrixRotationQuaternion(target) * XMMatrixTranslationFromVector(pos);
            const SceneComponent* parent = sc->GetParent();
            const XMMATRIX parentWorldInv = parent
                ? XMMatrixInverse(nullptr, parent->GetWorldTransform())
                : XMMatrixIdentity();
            localMat = world * parentWorldInv;
        }
        else
        {
            localMat =
                XMMatrixScalingFromVector(scl) * XMMatrixRotationQuaternion(target) * XMMatrixTranslationFromVector(pos);
        }

        nlohmann::json err;
        if (!ExecuteMcpCommand(core, std::make_unique<EditorCommand_SetProperty>(
                scAssetId, scObjectId, "m_localTransform", nlohmann::json{}, MatrixToJson(localMat)), err))
            return err;
        return MakeMcpOk();
    }

    return StartTransformChannelAnimation(
        core, sc, scAssetId, scObjectId, "rotation", params["value"], worldSpace, duration);
}

// ─── Command: SetScale ───────────────────────────────────────────────────────

nlohmann::json McpSceneSystem::CommandSetScale(EditorCore& core, const nlohmann::json& params)
{
    using namespace DirectX;
    using namespace DirectX::SimpleMath;

    if (!params.contains("objectId"))
        return MakeMcpError("missing required param: objectId");
    if (!params.contains("value") || !params["value"].is_array() || params["value"].size() < 3)
        return MakeMcpError("required param 'value' must be [x,y,z]");

    SceneComponent* sc = ResolveSceneComponent(core, params["objectId"].get<std::string>());
    if (!sc)
        return MakeMcpError("object has no SceneComponent: " + params["objectId"].get<std::string>());

    const auto& v      = params["value"];
    const Vector3 target(v[0].get<float>(), v[1].get<float>(), v[2].get<float>());
    //const float duration = params.value("duration_seconds", kDefaultAnimationDurationSeconds);
    const float duration = kDefaultAnimationDurationSeconds;

    auto [scAssetId, scObjectId] = core.GetIdsForObject(sc);

    if (duration <= 0.0f)
    {
        const Vector3    pos = sc->GetLocalPosition();
        const Quaternion rot = sc->GetLocalRotation();

        const XMMATRIX localMat =
            XMMatrixScalingFromVector(target) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos);

        nlohmann::json err;
        if (!ExecuteMcpCommand(core, std::make_unique<EditorCommand_SetProperty>(
                scAssetId, scObjectId, "m_localTransform", nlohmann::json{}, MatrixToJson(localMat)), err))
            return err;
        return MakeMcpOk();
    }

    return StartTransformChannelAnimation(
        core, sc, scAssetId, scObjectId, "scale", params["value"], /*worldSpace*/ false, duration);
}
