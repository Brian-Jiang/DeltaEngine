#include "Editor/Commands/EditorCommand_DeleteGameObject.h"
#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/EditorCore.h"

#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Serialization/ObjectSnapshotWriter.h"
#include "Runtime/Serialization/ObjectSnapshotReader.h"

#include <cstdio>

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
    DObject* obj = ctx.core.ResolveObject(m_assetId, m_gameObjectId);
    auto* go = dynamic_cast<GameObject*>(obj);
    if (!go)
        return false;

    ObjectSnapshotWriter writer;
    m_snapshot = writer.Capture(go);

    DWorld* world = ctx.core.GetWorld();
    if (!world)
        return false;

    world->DestroyGameObject(go);
    ctx.core.NotifyObjectDestroyed(m_gameObjectId);
    return true;
}

bool EditorCommand_DeleteGameObject::Undo(EditorCommandContext& ctx)
{
    DWorld* world = ctx.core.GetWorld();
    if (!world)
        return false;

    ObjectSnapshotReader reader;
    DObject* restored = reader.Restore(m_snapshot, world, ctx.core.GetAssetDatabase());

    if (!restored)
    {
        std::printf("EditorCommand_DeleteGameObject::Undo: restore failed\n");
        return false;
    }

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

void EditorCommand_DeleteGameObject::Deserialize(const nlohmann::json& in)
{
    m_assetId = UUID::FromString(in.value("assetId", ""));
    m_gameObjectId = UUID::FromString(in.value("gameObjectId", ""));
    m_snapshot.rootJson = in.value("snapshot", nlohmann::json{});
    m_snapshot.rootClassName = in.value("rootClassName", "");

    m_snapshot.capturedIds.clear();
    if (in.contains("capturedIds"))
    {
        for (const auto& idStr : in["capturedIds"])
            m_snapshot.capturedIds.push_back(UUID::FromString(idStr.get<std::string>()));
    }

    m_description.clear();
}
