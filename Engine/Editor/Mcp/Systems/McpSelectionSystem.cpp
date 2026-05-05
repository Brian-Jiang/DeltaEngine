#include "McpSelectionSystem.h"

#include "EditorCore.h"
#include "EditorSelectionState.h"
#include "Assets/EditorAssetDatabase.h"
#include "Mcp/McpRegistry.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Core/DComponent.h"
#include "Runtime/Reflection/DClass.h"

using namespace DeltaEngine;

void McpSelectionSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterOperation("selection", "current",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryCurrent(c, p); });
    registry.RegisterOperation("selection", "SelectObject",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandSelectObject(c, p); });
}

static DObject* FindObjectByObjectId(EditorCore& core, const ObjectId& id)
{
    DWorld* world = core.GetWorld();
    if (!world)
        return nullptr;

    for (GameObject* go : world->GetGameObjects())
    {
        auto [aId, oId] = core.GetIdsForObject(go);
        if (oId == id)
            return go;

        for (DComponent* comp : go->GetComponents())
        {
            auto [ca, co] = core.GetIdsForObject(comp);
            if (co == id)
                return comp;
        }
        for (SceneComponent* sc : go->GetSceneComponents())
        {
            auto [sa, so] = core.GetIdsForObject(sc);
            if (so == id)
                return sc;
        }
    }
    return nullptr;
}

nlohmann::json McpSelectionSystem::QueryCurrent(EditorCore& core, const nlohmann::json&)
{
    auto* sel = core.GetSelectionState();
    if (!sel)
        return {{"ok", false}, {"error", "no selection state"}};

    nlohmann::json gameObjects = nlohmann::json::array();
    for (const ObjectId& id : sel->GetSelectedGameObjects())
    {
        nlohmann::json entry;
        entry["object_id"] = id.ToString();
        if (DObject* obj = FindObjectByObjectId(core, id))
        {
            if (auto* go = dynamic_cast<GameObject*>(obj))
                entry["name"] = go->GetName();
            if (DClass* cls = obj->GetClass())
                entry["class"] = cls->GetName();
        }
        gameObjects.push_back(std::move(entry));
    }

    nlohmann::json components = nlohmann::json::array();
    for (const ObjectId& id : sel->GetSelectedComponents())
    {
        nlohmann::json entry;
        entry["object_id"] = id.ToString();
        if (DObject* obj = FindObjectByObjectId(core, id))
        {
            if (DClass* cls = obj->GetClass())
                entry["class"] = cls->GetName();
        }
        components.push_back(std::move(entry));
    }

    nlohmann::json assets = nlohmann::json::array();
    auto* assetDb = core.GetAssetDatabase();
    for (const AssetId& id : sel->GetSelectedAssets())
    {
        nlohmann::json entry;
        entry["asset_id"] = id.ToString();
        if (assetDb)
        {
            if (const auto* header = assetDb->GetAssetHeader(id))
                entry["class"] = header->m_className;
            auto path = assetDb->GetAssetPath(id);
            if (!path.empty())
                entry["path"] = path.string();
        }
        assets.push_back(std::move(entry));
    }

    return {
        {"ok", true},
        {"game_objects", std::move(gameObjects)},
        {"components", std::move(components)},
        {"assets", std::move(assets)}
    };
}

nlohmann::json McpSelectionSystem::CommandSelectObject(EditorCore& core, const nlohmann::json& params)
{
    auto* sel = core.GetSelectionState();
    if (!sel)
        return {{"ok", false}, {"error", "no selection state"}};

    if (!params.contains("object_ids") || !params["object_ids"].is_array())
        return {{"ok", false}, {"error", "missing or invalid object_ids"}};

    const bool addToSelection = params.value("add_to_selection", false);
    const auto& idsJson = params["object_ids"];

    if (idsJson.empty())
    {
        if (!addToSelection)
        {
            sel->ClearGameObjectSelection();
            sel->ClearComponentSelection();
            sel->ClearAssetSelection();
        }
        return {{"ok", true}};
    }

    enum class Kind : uint8_t { GameObject, Component, Asset };
    struct Resolved
    {
        Kind kind;
        ObjectId id;
    };
    std::vector<Resolved> resolved;
    nlohmann::json unknown = nlohmann::json::array();

    for (const auto& el : idsJson)
    {
        if (!el.is_string())
        {
            unknown.push_back(el.dump());
            continue;
        }
        const std::string s = el.get<std::string>();
        const ObjectId oid = DeltaEngine::UUID::FromString(s);
        if (oid.IsNull())
        {
            unknown.push_back(s);
            continue;
        }

        if (DObject* obj = FindObjectByObjectId(core, oid))
        {
            if (dynamic_cast<GameObject*>(obj))
                resolved.push_back({Kind::GameObject, oid});
            else if (dynamic_cast<DComponent*>(obj))
                resolved.push_back({Kind::Component, oid});
            else
                unknown.push_back(s);
            continue;
        }

        if (auto* adb = core.GetAssetDatabase())
        {
            if (adb->GetAssetHeader(oid))
            {
                resolved.push_back({Kind::Asset, oid});
                continue;
            }
        }

        unknown.push_back(s);
    }

    if (!addToSelection)
    {
        sel->ClearGameObjectSelection();
        sel->ClearComponentSelection();
        sel->ClearAssetSelection();
    }

    for (const Resolved& r : resolved)
    {
        switch (r.kind)
        {
        case Kind::GameObject: sel->AddSelectedGameObject(r.id); break;
        case Kind::Component: sel->AddSelectedComponent(r.id); break;
        case Kind::Asset: sel->AddSelectedAsset(r.id); break;
        default: DELTA_UNREACHABLE();
        }
    }

    nlohmann::json out{{"ok", true}, {"count", resolved.size()}};
    if (!unknown.empty())
        out["unknown_object_ids"] = std::move(unknown);
    return out;
}
