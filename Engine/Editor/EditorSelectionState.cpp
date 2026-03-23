#include "Editor/EditorSelectionState.h"

#include "Editor/EditorCore.h"
#include "Runtime/Core/DComponent.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Core/GameObject.h"

using namespace DeltaEngine;

void EditorSelectionState::SetSelection(const AssetId& assetId, const ObjectId& objectId)
{
    m_selectedAssetId   = assetId;
    m_selectedObjectId  = objectId;
    m_cachedObject      = nullptr;
}

void EditorSelectionState::SelectAsset(const AssetId& assetId)
{
    SetSelection(assetId, ObjectId::Null());
}

void EditorSelectionState::ClearSelection()
{
    m_selectedAssetId  = AssetId::Null();
    m_selectedObjectId = ObjectId::Null();
    m_cachedObject     = nullptr;
}

bool EditorSelectionState::HasSelection() const
{
    return !m_selectedAssetId.IsNull();
}

bool EditorSelectionState::HasAssetSelection() const
{
    return !m_selectedAssetId.IsNull() && m_selectedObjectId.IsNull();
}

DObject* EditorSelectionState::ResolveSelection(EditorCore& core)
{
    if (m_selectedAssetId.IsNull() || m_selectedObjectId.IsNull())
        return nullptr;

    if (m_cachedObject)
        return m_cachedObject;

    DObject* found = core.FindObject(m_selectedAssetId, m_selectedObjectId);
    if (!found)
    {
        ClearSelection();
        return nullptr;
    }

    m_cachedObject = found;
    return m_cachedObject;
}

GameObject* EditorSelectionState::GetSelectedGameObject(EditorCore& core)
{
    return dynamic_cast<GameObject*>(ResolveSelection(core));
}

DComponent* EditorSelectionState::GetSelectedComponent(EditorCore& core)
{
    return dynamic_cast<DComponent*>(ResolveSelection(core));
}

GameObject* EditorSelectionState::GetContextGameObject(EditorCore& core)
{
    DObject* obj = ResolveSelection(core);
    if (!obj)
        return nullptr;

    if (auto* go = dynamic_cast<GameObject*>(obj))
        return go;

    if (auto* comp = dynamic_cast<DComponent*>(obj))
        return comp->GetGameObject();

    return nullptr;
}

void EditorSelectionState::NotifyObjectDestroyed(const ObjectId& objectId)
{
    if (objectId.IsNull())
        return;

    if (m_selectedObjectId == objectId)
        ClearSelection();
}
