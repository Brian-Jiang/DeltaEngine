#include "Editor/Commands/EditorCommand_DuplicateGameObject.h"
#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/EditorCore.h"
#include "Editor/EditorSelectionState.h"

#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Assets/PA_DScene.h"
#include "Runtime/Core/DScene.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Serialization/ObjectSnapshotReader.h"
#include "Runtime/Serialization/ObjectSnapshotWriter.h"

#include <format>
#include <unordered_map>

using namespace DeltaEngine;

namespace
{
    // Recursively replaces every string value that matches a captured id with its remapped id.
    void RemapIdStrings(nlohmann::json& node, const std::unordered_map<std::string, std::string>& remap)
    {
        if (node.is_object())
        {
            for (auto& [key, value] : node.items())
                RemapIdStrings(value, remap);
        }
        else if (node.is_array())
        {
            for (auto& element : node)
                RemapIdStrings(element, remap);
        }
        else if (node.is_string())
        {
            auto it = remap.find(node.get<std::string>());
            if (it != remap.end())
                node = it->second;
        }
    }
}

EditorCommand_DuplicateGameObject::EditorCommand_DuplicateGameObject(
    AssetId assetId, ObjectId sourceGameObjectId, std::string newName,
    std::optional<std::array<float, 3>> offsetPosition)
    : m_assetId(assetId)
    , m_sourceGameObjectId(sourceGameObjectId)
    , m_newName(std::move(newName))
{
    if (offsetPosition)
    {
        m_offset[0] = (*offsetPosition)[0];
        m_offset[1] = (*offsetPosition)[1];
        m_offset[2] = (*offsetPosition)[2];
        m_hasOffset = true;
    }
}

std::string_view EditorCommand_DuplicateGameObject::GetDescription() const
{
    if (m_description.empty())
        m_description = "Duplicate GameObject";
    return m_description;
}

bool EditorCommand_DuplicateGameObject::Execute(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Duplicate GameObject] Execute: Start");

    DObject* obj = ctx.core.ResolveObject(m_assetId, m_sourceGameObjectId);
    auto* source = dynamic_cast<GameObject*>(obj);
    if (!source)
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[Duplicate GameObject] Execute: source GameObject {} not found in asset {}",
             m_sourceGameObjectId.ToString(), m_assetId.ToString());
        return false;
    }

    ObjectSnapshotWriter writer;
    ObjectSnapshot captured = writer.Capture(source);

    // Remap every captured id to a fresh id so the duplicate is an independent object graph.
    std::unordered_map<std::string, std::string> remap;
    m_snapshot.capturedIds.clear();
    m_snapshot.capturedIds.reserve(captured.capturedIds.size());
    for (const ObjectId& oldId : captured.capturedIds)
    {
        ObjectId freshId = ObjectId::Generate();
        remap[oldId.ToString()] = freshId.ToString();
        m_snapshot.capturedIds.push_back(freshId);
    }

    m_snapshot.rootClassName = captured.rootClassName;
    m_snapshot.rootJson = std::move(captured.rootJson);
    RemapIdStrings(m_snapshot.rootJson, remap);

    // Bake the new name into the root GameObject before restore, if requested.
    if (!m_newName.empty() && m_snapshot.rootJson.contains("objects") &&
        m_snapshot.rootJson["objects"].is_array() && !m_snapshot.rootJson["objects"].empty())
    {
        m_snapshot.rootJson["objects"][0]["m_name"] = m_newName;
    }

    m_hasSnapshot = true;

    return RestoreDuplicate(ctx);
}

bool EditorCommand_DuplicateGameObject::Redo(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Duplicate GameObject] Redo: Start");

    if (!m_hasSnapshot)
        return Execute(ctx);

    return RestoreDuplicate(ctx);
}

bool EditorCommand_DuplicateGameObject::RestoreDuplicate(EditorCommandContext& ctx)
{
    DWorld* world = ctx.core.GetWorld();
    if (!world)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Duplicate GameObject] No world");
        return false;
    }

    EditorAssetDatabase* db = ctx.core.GetAssetDatabase();
    if (!db)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Duplicate GameObject] No asset database");
        return false;
    }

    DPrimaryAsset* asset = db->GetLoadedAsset(m_assetId);
    if (!asset)
        asset = db->LoadAsset(m_assetId);
    if (!asset)
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[Duplicate GameObject] Failed to load scene asset {}", m_assetId.ToString());
        return false;
    }

    DScene* scene = nullptr;
    if (auto* pa = dynamic_cast<PA_DScene*>(asset))
        scene = pa->GetScene();

    ObjectSnapshotReader reader;
    DObject* restored = reader.Restore(m_snapshot, world, db, asset, scene);

    auto* go = dynamic_cast<GameObject*>(restored);
    if (!go)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Duplicate GameObject] Failed to restore duplicate from snapshot");
        return false;
    }

    m_createdId = go->GetObjectId();

    if (m_hasOffset)
    {
        if (SceneComponent* root = go->GetRootSceneComponent())
        {
            const DirectX::SimpleMath::Vector3 offset(m_offset[0], m_offset[1], m_offset[2]);
            root->SetLocalPosition(root->GetLocalPosition() + offset);
        }
    }

    if (EditorSelectionState* sel = ctx.core.GetSelectionState())
        sel->SetSelectedGameObject(m_createdId);

    return true;
}

bool EditorCommand_DuplicateGameObject::Undo(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Duplicate GameObject] Undo: Start");

    DWorld* world = ctx.core.GetWorld();
    if (!world)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Duplicate GameObject] Undo: No world");
        return false;
    }

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
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[Duplicate GameObject] Undo: duplicate {} not found", m_createdId.ToString());
        return false;
    }

    world->DestroyGameObject(go);
    ctx.core.NotifyObjectDestroyed(m_createdId);
    return true;
}

void EditorCommand_DuplicateGameObject::Serialize(nlohmann::json& out) const
{
    out["assetId"] = m_assetId.ToString();
    out["sourceObjectId"] = m_sourceGameObjectId.ToString();
    out["newName"] = m_newName;
    out["createdId"] = m_createdId.ToString();

    if (m_hasOffset)
        out["offsetPosition"] = {m_offset[0], m_offset[1], m_offset[2]};

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

void EditorCommand_DuplicateGameObject::Deserialize(const nlohmann::json& in)
{
    m_assetId = UUID::FromString(in.value("assetId", ""));
    m_sourceGameObjectId = UUID::FromString(in.value("sourceObjectId", ""));
    m_newName = in.value("newName", "");
    m_createdId = UUID::FromString(in.value("createdId", ""));

    m_hasOffset = false;
    if (in.contains("offsetPosition") && in["offsetPosition"].is_array() && in["offsetPosition"].size() >= 3)
    {
        m_offset[0] = in["offsetPosition"][0].get<float>();
        m_offset[1] = in["offsetPosition"][1].get<float>();
        m_offset[2] = in["offsetPosition"][2].get<float>();
        m_hasOffset = true;
    }

    m_hasSnapshot = false;
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
