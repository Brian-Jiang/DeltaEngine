#include "Editor/Commands/EditorCommand_CreateComponent.h"
#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/EditorCore.h"
#include "Editor/EditorSelectionState.h"

#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/DComponent.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Serialization/ObjectSnapshotWriter.h"
#include "Runtime/Serialization/ObjectSnapshotReader.h"

#include <algorithm>
#include <cstdio>
#include <format>

using namespace DeltaEngine;

EditorCommand_CreateComponent::EditorCommand_CreateComponent(
    AssetId sceneAssetId, ObjectId gameObjectId, std::string componentClassName)
    : m_sceneAssetId(sceneAssetId)
    , m_gameObjectId(gameObjectId)
    , m_className(std::move(componentClassName))
{
}

std::string_view EditorCommand_CreateComponent::GetDescription() const
{
    if (m_description.empty())
        m_description = std::format("Add Component: {}", m_className);
    return m_description;
}

bool EditorCommand_CreateComponent::Execute(EditorCommandContext& ctx)
{
    DPrimaryAsset* asset = ctx.core.GetActiveSceneAsset();
    if (!asset)
        return false;

    DObject* obj = asset->FindObject(m_gameObjectId);
    GameObject* go = dynamic_cast<GameObject*>(obj);
    if (!go)
        return false;

    const DClass* dclass = GetReflectionRegistry().FindClassByName(m_className);
    if (!dclass)
        return false;

    DComponent* comp = go->AddComponentByClass(dclass);
    if (!comp)
        return false;

    asset->MarkDirty();
    m_createdComponentId = comp->GetObjectId();

    auto* sc = dynamic_cast<SceneComponent*>(comp);
    m_isSceneComponent = (sc != nullptr);

    if (m_isSceneComponent)
    {
        m_componentIndex = static_cast<int>(go->GetSceneComponents().size()) - 1;
        if (sc->GetParent())
            m_parentSceneComponentId = sc->GetParent()->GetObjectId();
    }
    else
    {
        m_componentIndex = static_cast<int>(go->GetComponents().size()) - 1;
    }

    if (EditorSelectionState* sel = ctx.core.GetSelectionState())
        sel->SetSelectedComponent(m_createdComponentId);

    return true;
}

bool EditorCommand_CreateComponent::Undo(EditorCommandContext& ctx)
{
    DPrimaryAsset* asset = ctx.core.GetActiveSceneAsset();
    if (!asset)
        return false;

    DObject* goObj = asset->FindObject(m_gameObjectId);
    GameObject* go = dynamic_cast<GameObject*>(goObj);
    if (!go)
        return false;

    DObject* compObj = asset->FindObject(m_createdComponentId);
    DComponent* comp = dynamic_cast<DComponent*>(compObj);
    if (!comp)
        return false;

    ObjectSnapshotWriter writer;
    m_snapshot = writer.Capture(comp);
    m_hasSnapshot = true;

    go->RemoveComponent(comp);
    ctx.core.NotifyObjectDestroyed(m_createdComponentId);
    asset->MarkDirty();
    return true;
}

bool EditorCommand_CreateComponent::Redo(EditorCommandContext& ctx)
{
    if (!m_hasSnapshot)
        return Execute(ctx);

    DPrimaryAsset* asset = ctx.core.GetActiveSceneAsset();
    if (!asset)
        return false;

    DObject* goObj = asset->FindObject(m_gameObjectId);
    GameObject* go = dynamic_cast<GameObject*>(goObj);
    if (!go)
        return false;

    ObjectSnapshotReader reader;
    DObject* restored = reader.Restore(m_snapshot, nullptr, ctx.core.GetAssetDatabase(), asset, nullptr);
    if (!restored)
        return false;

    m_createdComponentId = restored->GetObjectId();

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
        sel->SetSelectedComponent(m_createdComponentId);

    return true;
}

void EditorCommand_CreateComponent::Serialize(nlohmann::json& out) const
{
    out["sceneAssetId"] = m_sceneAssetId.ToString();
    out["gameObjectId"] = m_gameObjectId.ToString();
    out["className"] = m_className;
    out["createdComponentId"] = m_createdComponentId.ToString();
    out["isSceneComponent"] = m_isSceneComponent;
    out["componentIndex"] = m_componentIndex;
    out["parentSceneComponentId"] = m_parentSceneComponentId.ToString();

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

void EditorCommand_CreateComponent::Deserialize(const nlohmann::json& in)
{
    m_sceneAssetId = UUID::FromString(in.value("sceneAssetId", ""));
    m_gameObjectId = UUID::FromString(in.value("gameObjectId", ""));
    m_className = in.value("className", "");
    m_createdComponentId = UUID::FromString(in.value("createdComponentId", ""));
    m_isSceneComponent = in.value("isSceneComponent", false);
    m_componentIndex = in.value("componentIndex", -1);
    m_parentSceneComponentId = UUID::FromString(in.value("parentSceneComponentId", ""));

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
