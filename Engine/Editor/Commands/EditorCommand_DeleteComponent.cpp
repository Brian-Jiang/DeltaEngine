#include "Editor/Commands/EditorCommand_DeleteComponent.h"
#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/EditorCore.h"
#include "Editor/EditorSelectionState.h"

#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/DComponent.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Serialization/ObjectSnapshotWriter.h"
#include "Runtime/Serialization/ObjectSnapshotReader.h"

#include <algorithm>
#include <cstdio>

using namespace DeltaEngine;

EditorCommand_DeleteComponent::EditorCommand_DeleteComponent(
    AssetId sceneAssetId, ObjectId gameObjectId, ObjectId componentId)
    : m_sceneAssetId(sceneAssetId)
    , m_gameObjectId(gameObjectId)
    , m_componentId(componentId)
{
}

std::string_view EditorCommand_DeleteComponent::GetDescription() const
{
    if (m_description.empty())
        m_description = "Delete Component";
    return m_description;
}

bool EditorCommand_DeleteComponent::Execute(EditorCommandContext& ctx)
{
    DPrimaryAsset* asset = ctx.core.GetActiveSceneAsset();
    if (!asset)
        return false;

    DObject* goObj = asset->FindObject(m_gameObjectId);
    GameObject* go = dynamic_cast<GameObject*>(goObj);
    if (!go)
        return false;

    DObject* compObj = asset->FindObject(m_componentId);
    DComponent* comp = dynamic_cast<DComponent*>(compObj);
    if (!comp)
        return false;

    auto* sc = dynamic_cast<SceneComponent*>(comp);
    m_isSceneComponent = (sc != nullptr);

    if (m_isSceneComponent)
    {
        if (sc == go->GetRootSceneComponent() && go->GetSceneComponents().size() <= 1)
        {
            std::printf("[DeleteComponent] Cannot delete the last root SceneComponent.\n");
            return false;
        }

        const auto& scList = go->GetSceneComponents();
        auto scIt = std::find(scList.begin(), scList.end(), sc);
        m_componentIndex = (scIt != scList.end()) ? static_cast<int>(scIt - scList.begin()) : -1;

        if (sc->GetParent())
            m_parentSceneComponentId = sc->GetParent()->GetObjectId();
    }
    else
    {
        const auto& cList = go->GetComponents();
        auto cIt = std::find(cList.begin(), cList.end(), comp);
        m_componentIndex = (cIt != cList.end()) ? static_cast<int>(cIt - cList.begin()) : -1;
    }

    ObjectSnapshotWriter writer;
    m_snapshot = writer.Capture(comp);

    go->RemoveComponent(comp);
    ctx.core.NotifyObjectDestroyed(m_componentId);
    asset->MarkDirty();
    return true;
}

bool EditorCommand_DeleteComponent::Undo(EditorCommandContext& ctx)
{
    DPrimaryAsset* asset = ctx.core.GetActiveSceneAsset();
    if (!asset)
        return false;

    DObject* goObj = asset->FindObject(m_gameObjectId);
    GameObject* go = dynamic_cast<GameObject*>(goObj);
    if (!go)
        return false;

    ObjectSnapshotReader reader;
    DObject* restored = reader.Restore(m_snapshot, nullptr, ctx.core.GetAssetDatabase());
    if (!restored)
        return false;

    m_componentId = restored->GetObjectId();

    if (m_isSceneComponent)
    {
        auto* sc = dynamic_cast<SceneComponent*>(restored);
        if (!sc)
            return false;

        SceneComponent* parent = nullptr;
        if (!m_parentSceneComponentId.IsNull())
        {
            DObject* pObj = asset->FindObject(m_parentSceneComponentId);
            parent = dynamic_cast<SceneComponent*>(pObj);
        }
        go->InsertSceneComponent(sc, m_componentIndex, parent);
        sc->PostRestore();
    }
    else
    {
        auto* comp = dynamic_cast<DComponent*>(restored);
        if (!comp)
            return false;
        go->InsertComponent(comp, m_componentIndex);
        comp->PostRestore();
    }

    asset->MarkDirty();

    if (EditorSelectionState* sel = ctx.core.GetSelectionState())
        sel->SetSelectedComponent(m_componentId);

    return true;
}

bool EditorCommand_DeleteComponent::Redo(EditorCommandContext& ctx)
{
    return Execute(ctx);
}

void EditorCommand_DeleteComponent::Serialize(nlohmann::json& out) const
{
    out["sceneAssetId"] = m_sceneAssetId.ToString();
    out["gameObjectId"] = m_gameObjectId.ToString();
    out["componentId"] = m_componentId.ToString();
    out["isSceneComponent"] = m_isSceneComponent;
    out["componentIndex"] = m_componentIndex;
    out["parentSceneComponentId"] = m_parentSceneComponentId.ToString();

    out["snapshot"] = m_snapshot.rootJson;
    out["rootClassName"] = m_snapshot.rootClassName;
    nlohmann::json ids = nlohmann::json::array();
    for (const auto& id : m_snapshot.capturedIds)
        ids.push_back(id.ToString());
    out["capturedIds"] = ids;
}

void EditorCommand_DeleteComponent::Deserialize(const nlohmann::json& in)
{
    m_sceneAssetId = UUID::FromString(in.value("sceneAssetId", ""));
    m_gameObjectId = UUID::FromString(in.value("gameObjectId", ""));
    m_componentId = UUID::FromString(in.value("componentId", ""));
    m_isSceneComponent = in.value("isSceneComponent", false);
    m_componentIndex = in.value("componentIndex", -1);
    m_parentSceneComponentId = UUID::FromString(in.value("parentSceneComponentId", ""));

    m_snapshot.rootJson = in.value("snapshot", nlohmann::json{});
    m_snapshot.rootClassName = in.value("rootClassName", "");
    m_snapshot.capturedIds.clear();
    if (in.contains("capturedIds"))
        for (const auto& idStr : in["capturedIds"])
            m_snapshot.capturedIds.push_back(UUID::FromString(idStr.get<std::string>()));

    m_description.clear();
}
