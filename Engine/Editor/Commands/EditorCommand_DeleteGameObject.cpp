#include "Editor/Commands/EditorCommand_DeleteGameObject.h"
#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/EditorCore.h"
#include "Runtime/Assets/PA_DScene.h"
#include "Editor/EditorSelectionState.h"

#include "Runtime/Core/DScene.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Serialization/ObjectSnapshotWriter.h"
#include "Runtime/Serialization/ObjectSnapshotReader.h"

using namespace DeltaEngine;

EditorCommand_DeleteGameObject::EditorCommand_DeleteGameObject(
    AssetId assetId, ObjectId gameObjectId)
    : m_assetId(assetId)
    , m_gameObjectId(gameObjectId)
{
}

std::string_view EditorCommand_DeleteGameObject::GetDescription() const
{
    if (m_description.empty())
        m_description = "Delete GameObject";
    return m_description;
}

bool EditorCommand_DeleteGameObject::Execute(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Delete GameObject] Execute: Start");

    DObject* obj = ctx.core.ResolveObject(m_assetId, m_gameObjectId);
    auto* go = dynamic_cast<GameObject*>(obj);
    if (!go)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Delete GameObject] Execute: GameObject with ID {} not found, assetId {}", m_gameObjectId.ToString(), m_assetId.ToString());
        return false;
    }

    ObjectSnapshotWriter writer;
    m_snapshot = writer.Capture(go);

    DWorld* world = ctx.core.GetWorld();
    if (!world)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Delete GameObject] Execute: No world");
        return false;
    }

    world->DestroyGameObject(go);
    ctx.core.NotifyObjectDestroyed(m_gameObjectId);
    return true;
}

bool EditorCommand_DeleteGameObject::Undo(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Delete GameObject] Undo: Start");

    DWorld* world = ctx.core.GetWorld();
    if (!world)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Delete GameObject] Undo: No world");
        return false;
    }

    EditorAssetDatabase* db = ctx.core.GetAssetDatabase();
    if (!db)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Delete GameObject] Undo: No asset database");
        return false;
    }

    DPrimaryAsset* asset = db->GetLoadedAsset(m_assetId);
    if (!asset)
        asset = db->LoadAsset(m_assetId);
    if (!asset)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Delete GameObject] Undo: Failed to load asset {}", m_assetId.ToString());
        return false;
    }

    DScene* scene = nullptr;
    if (auto* pa = dynamic_cast<PA_DScene*>(asset))
        scene = pa->GetScene();

    ObjectSnapshotReader reader;
    DObject* restored = reader.Restore(m_snapshot, world, db, asset, scene);

    if (!restored)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Delete GameObject] Undo: Failed to restore GameObject from snapshot");
        return false;
    }

    if (EditorSelectionState* sel = ctx.core.GetSelectionState())
        sel->SetSelectedGameObject(m_gameObjectId);

    return true;
}

void EditorCommand_DeleteGameObject::Serialize(nlohmann::json& out) const
{
    out["assetId"] = m_assetId.ToString();
    out["gameObjectId"] = m_gameObjectId.ToString();
    out["snapshot"] = m_snapshot.rootJson;
    out["rootClassName"] = m_snapshot.rootClassName;

    nlohmann::json ids = nlohmann::json::array();
    for (const auto& id : m_snapshot.capturedIds)
        ids.push_back(id.ToString());
    out["capturedIds"] = ids;
}
