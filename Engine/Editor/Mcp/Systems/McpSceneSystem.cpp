#include "McpSceneSystem.h"

#include "Editor/EditorCore.h"
#include "Editor/Commands/PropertyValueIO.h"
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
    default:
        DELTA_UNREACHABLE();
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

    if (name.empty() || name == "New GameObject")
    {
        nlohmann::json data;
        data["className"] = "GameObject";
        return EnqueueMcpCommand(core, "scene", "EditorCommand_CreateGameObject", std::move(data), true);
    }

    nlohmann::json envelope;
    envelope["type"] = "auxiliary";
    envelope["name"] = "CreateGameObjectWithRename";
    envelope["desiredName"] = name;
    core.EnqueueSerializedCommand(envelope.dump());
    return {
        {"ok", true},
        {"queued", true},
        {"command", "CreateGameObjectWithRename"},
        {"expects_result", true}
    };
}

// ─── Command: DeleteGameObject ──────────────────────────────────────────────

nlohmann::json McpSceneSystem::CommandDeleteGameObject(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("objectId"))
        return MakeMcpError("missing required param: objectId");

    nlohmann::json data;
    data["gameObjectId"] = params["objectId"].get<std::string>();
    return EnqueueMcpCommand(core, "scene", "EditorCommand_DeleteGameObject", std::move(data), true);
}

// ─── Command: DuplicateGameObject ───────────────────────────────────────────

nlohmann::json McpSceneSystem::CommandDuplicateGameObject(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("objectId"))
        return MakeMcpError("missing required param: objectId");

    nlohmann::json data;
    data["sourceObjectId"] = params["objectId"].get<std::string>();
    if (params.contains("newName") && params["newName"].is_string())
        data["newName"] = params["newName"].get<std::string>();
    if (params.contains("offset_position") && params["offset_position"].is_array())
        data["offsetPosition"] = params["offset_position"];

    return EnqueueMcpCommand(core, "scene", "EditorCommand_DuplicateGameObject", std::move(data), true);
}

// ─── Command: ReparentSceneComponent ────────────────────────────────────────

nlohmann::json McpSceneSystem::CommandReparentSceneComponent(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("objectId"))
        return MakeMcpError("missing required param: objectId");
    if (!params.contains("newParentId"))
        return MakeMcpError("missing required param: newParentId");

    nlohmann::json data;
    data["childObjectId"] = params["objectId"].get<std::string>();
    data["newParentObjectId"] = params["newParentId"].get<std::string>();
    return EnqueueMcpCommand(core, "scene", "EditorCommand_ReparentSceneComponent", std::move(data), true);
}

// ─── Command: CreateComponent ───────────────────────────────────────────────

nlohmann::json McpSceneSystem::CommandCreateComponent(EditorCore& core, const nlohmann::json& params)
{
    if (!params.contains("objectId"))
        return MakeMcpError("missing required param: objectId");
    if (!params.contains("componentClass"))
        return MakeMcpError("missing required param: componentClass");

    nlohmann::json data;
    data["gameObjectId"] = params["objectId"].get<std::string>();
    data["className"] = params["componentClass"].get<std::string>();
    return EnqueueMcpCommand(core, "scene", "EditorCommand_CreateComponent", std::move(data), true);
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

    //auto [ownerAssetId, ownerObjectId] = core.GetIdsForObject(owner);
    auto ownerObjectId = comp->GetGameObject()->GetObjectId();

    nlohmann::json data;
    data["gameObjectId"] = ownerObjectId.ToString();
    data["componentId"] = compIdStr;
    return EnqueueMcpCommand(core, "scene", "EditorCommand_DeleteComponent", std::move(data), true);
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
    const float duration    = params.value("duration_seconds", kDefaultAnimationDurationSeconds);

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

        nlohmann::json data;
        data["objectId"]      = scObjectId.ToString();
        data["propertyName"]  = "m_localTransform";
        data["valueAfter"]    = MatrixToJson(localMat);
        return EnqueueMcpCommand(core, "scene", "EditorCommand_SetProperty", std::move(data), true);
    }

    // Animation path — resolved IDs stored in the auxiliary so DrainCommandQueue can act.
    nlohmann::json envelope;
    envelope["type"]      = "auxiliary";
    envelope["name"]      = "StartTransformChannelAnimation";
    envelope["scAssetId"] = scAssetId.ToString();
    envelope["scObjectId"]= scObjectId.ToString();
    envelope["channel"]   = "position";
    envelope["targetValue"] = params["value"];
    envelope["space"]     = space;
    envelope["duration"]  = duration;
    core.EnqueueSerializedCommand(envelope.dump());
    return {{"ok", true}, {"queued", true}, {"expects_result", false}};
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
    const float duration    = params.value("duration_seconds", kDefaultAnimationDurationSeconds);

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

        nlohmann::json data;
        data["objectId"]     = scObjectId.ToString();
        data["propertyName"] = "m_localTransform";
        data["valueAfter"]   = MatrixToJson(localMat);
        return EnqueueMcpCommand(core, "scene", "EditorCommand_SetProperty", std::move(data), true);
    }

    nlohmann::json envelope;
    envelope["type"]       = "auxiliary";
    envelope["name"]       = "StartTransformChannelAnimation";
    envelope["scAssetId"]  = scAssetId.ToString();
    envelope["scObjectId"] = scObjectId.ToString();
    envelope["channel"]    = "rotation";
    envelope["targetValue"]= params["value"];
    envelope["space"]      = space;
    envelope["duration"]   = duration;
    core.EnqueueSerializedCommand(envelope.dump());
    return {{"ok", true}, {"queued", true}, {"expects_result", false}};
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
    const float duration = params.value("duration_seconds", kDefaultAnimationDurationSeconds);

    auto [scAssetId, scObjectId] = core.GetIdsForObject(sc);

    if (duration <= 0.0f)
    {
        const Vector3    pos = sc->GetLocalPosition();
        const Quaternion rot = sc->GetLocalRotation();

        const XMMATRIX localMat =
            XMMatrixScalingFromVector(target) * XMMatrixRotationQuaternion(rot) * XMMatrixTranslationFromVector(pos);

        nlohmann::json data;
        data["objectId"]     = scObjectId.ToString();
        data["propertyName"] = "m_localTransform";
        data["valueAfter"]   = MatrixToJson(localMat);
        return EnqueueMcpCommand(core, "scene", "EditorCommand_SetProperty", std::move(data), true);
    }

    nlohmann::json envelope;
    envelope["type"]       = "auxiliary";
    envelope["name"]       = "StartTransformChannelAnimation";
    envelope["scAssetId"]  = scAssetId.ToString();
    envelope["scObjectId"] = scObjectId.ToString();
    envelope["channel"]    = "scale";
    envelope["targetValue"]= params["value"];
    envelope["space"]      = "local";
    envelope["duration"]   = duration;
    core.EnqueueSerializedCommand(envelope.dump());
    return {{"ok", true}, {"queued", true}, {"expects_result", false}};
}
