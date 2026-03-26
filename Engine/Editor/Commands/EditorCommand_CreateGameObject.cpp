#include "Editor/Commands/EditorCommand_CreateGameObject.h"
#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/EditorCore.h"
#include "Editor/EditorSelectionState.h"

#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Assets/PA_DScene.h"
#include "Runtime/Core/DScene.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Serialization/ObjectSnapshotWriter.h"
#include "Runtime/Serialization/ObjectSnapshotReader.h"

#include <cstdio>
#include <format>

using namespace DeltaEngine;

EditorCommand_CreateGameObject::EditorCommand_CreateGameObject(
    AssetId sceneAssetId, std::string className)
    : m_sceneAssetId(sceneAssetId)
    , m_className(std::move(className))
{
}

std::string_view EditorCommand_CreateGameObject::GetDescription() const
{
    if (m_description.empty())
        m_description = std::format("Create {}", m_className);
    return m_description;
}

bool EditorCommand_CreateGameObject::Execute(EditorCommandContext& ctx)
{
    DPrimaryAsset* asset = ctx.core.GetActiveSceneAsset();
    if (!asset)
        return false;

    DWorld* world = ctx.core.GetWorld();
    if (!world)
        return false;

    DScene* scene = world->GetActiveScene();
    const std::string goName = std::format("New {}", m_className);
    GameObject* go = world->CreateGameObjectInScene(scene, goName);
    if (!go)
        return false;

    if (SceneComponent* root = go->GetRootSceneComponent())
        if (!root->HasOwningAsset())
            asset->AddObject(root);

    m_createdId = go->GetObjectId();

    if (EditorSelectionState* sel = ctx.core.GetSelectionState())
        sel->SetSelectedGameObject(m_createdId);

    return true;
}

bool EditorCommand_CreateGameObject::Undo(EditorCommandContext& ctx)
{
    DWorld* world = ctx.core.GetWorld();
    if (!world)
        return false;

    GameObject* go = nullptr;
    for (auto* g : world->GetGameObjects())
    {
        if (g->GetObjectId() == m_createdId)
        {
            go = g;
            break;
        }
    }
    if (!go)
        return false;

    ObjectSnapshotWriter writer;
    m_snapshot = writer.Capture(go);
    m_hasSnapshot = true;

    world->DestroyGameObject(go);
    ctx.core.NotifyObjectDestroyed(m_createdId);
    return true;
}

bool EditorCommand_CreateGameObject::Redo(EditorCommandContext& ctx)
{
    if (!m_hasSnapshot)
        return Execute(ctx);

    DWorld* world = ctx.core.GetWorld();
    if (!world)
        return false;

    EditorAssetDatabase* db = ctx.core.GetAssetDatabase();
    if (!db)
        return false;

    DPrimaryAsset* asset = db->GetLoadedAsset(m_sceneAssetId);
    if (!asset)
        asset = db->LoadAsset(m_sceneAssetId);
    if (!asset)
        return false;

    DScene* scene = nullptr;
    if (auto* pa = dynamic_cast<PA_DScene*>(asset))
        scene = pa->GetScene();

    ObjectSnapshotReader reader;
    DObject* restored = reader.Restore(m_snapshot, world, db, asset, scene);
    if (!restored)
        return false;

    m_createdId = restored->GetObjectId();

    if (EditorSelectionState* sel = ctx.core.GetSelectionState())
        sel->SetSelectedGameObject(m_createdId);

    return true;
}

void EditorCommand_CreateGameObject::Serialize(nlohmann::json& out) const
{
    out["sceneAssetId"] = m_sceneAssetId.ToString();
    out["className"] = m_className;
    out["createdId"] = m_createdId.ToString();

    if (m_hasSnapshot)
    {
        out["snapshot"] = m_snapshot.rootJson;
        out["rootClassName"] = m_snapshot.rootClassName;
        nlohmann::json ids = nlohmann::json::array();
        for (const auto& id : m_snapshot.capturedIds)
            ids.push_back(id.ToString());
        out["capturedIds"] = ids;
    }
}

void EditorCommand_CreateGameObject::Deserialize(const nlohmann::json& in)
{
    m_sceneAssetId = UUID::FromString(in.value("sceneAssetId", ""));
    m_className = in.value("className", "");
    m_createdId = UUID::FromString(in.value("createdId", ""));

    if (in.contains("snapshot"))
    {
        m_snapshot.rootJson = in["snapshot"];
        m_snapshot.rootClassName = in.value("rootClassName", "");
        m_snapshot.capturedIds.clear();
        if (in.contains("capturedIds"))
            for (const auto& idStr : in["capturedIds"])
                m_snapshot.capturedIds.push_back(UUID::FromString(idStr.get<std::string>()));
        m_hasSnapshot = true;
    }

    m_description.clear();
}
