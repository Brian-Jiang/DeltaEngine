#include "Editor/Commands/EditorCommand_ReparentSceneComponent.h"
#include "Editor/EditorCore.h"

#include "Runtime/Core/SceneComponent.h"

using namespace DeltaEngine;

EditorCommand_ReparentSceneComponent::EditorCommand_ReparentSceneComponent(
    AssetId sceneAssetId, ObjectId childObjectId, ObjectId newParentObjectId)
    : m_sceneAssetId(sceneAssetId)
    , m_childObjectId(childObjectId)
    , m_newParentObjectId(newParentObjectId)
{
}

std::string_view EditorCommand_ReparentSceneComponent::GetDescription() const
{
    if (m_description.empty())
        m_description = "Reparent SceneComponent";
    return m_description;
}

bool EditorCommand_ReparentSceneComponent::ApplyReparent(EditorCommandContext& ctx, const ObjectId& newParentId)
{
    auto* child = ctx.core.ResolveObject<SceneComponent>(m_sceneAssetId, m_childObjectId);
    if (!child)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Reparent] Child {} not found", m_childObjectId.ToString());
        return false;
    }

    SceneComponent* newParent = nullptr;
    if (!newParentId.IsNull())
    {
        newParent = ctx.core.ResolveObject<SceneComponent>(m_sceneAssetId, newParentId);
        if (!newParent)
        {
            DLOG(LogEditorCommand, ELogLevel::Error, "[Reparent] New parent {} not found", newParentId.ToString());
            return false;
        }
    }

    child->SetParent(newParent);
    child->PostRestore();
    return true;
}

bool EditorCommand_ReparentSceneComponent::Execute(EditorCommandContext& ctx)
{
    auto* child = ctx.core.ResolveObject<SceneComponent>(m_sceneAssetId, m_childObjectId);
    if (!child)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Reparent] Execute: Child {} not found", m_childObjectId.ToString());
        return false;
    }

    if (SceneComponent* oldParent = child->GetParent())
        m_oldParentObjectId = oldParent->GetObjectId();
    else
        m_oldParentObjectId = ObjectId::Null();

    return ApplyReparent(ctx, m_newParentObjectId);
}

bool EditorCommand_ReparentSceneComponent::Undo(EditorCommandContext& ctx)
{
    return ApplyReparent(ctx, m_oldParentObjectId);
}

void EditorCommand_ReparentSceneComponent::Serialize(nlohmann::json& out) const
{
    out["sceneAssetId"] = m_sceneAssetId.ToString();
    out["childObjectId"] = m_childObjectId.ToString();
    out["newParentObjectId"] = m_newParentObjectId.ToString();
    out["oldParentObjectId"] = m_oldParentObjectId.ToString();
}

void EditorCommand_ReparentSceneComponent::Deserialize(const nlohmann::json& in)
{
    m_sceneAssetId = UUID::FromString(in.value("sceneAssetId", ""));
    m_childObjectId = UUID::FromString(in.value("childObjectId", ""));
    m_newParentObjectId = UUID::FromString(in.value("newParentObjectId", ""));
    m_oldParentObjectId = UUID::FromString(in.value("oldParentObjectId", ""));
    m_description.clear();
}
