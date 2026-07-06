#include "Editor/EditorSelectionState.h"

#include "Editor/EditorCore.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/GameObject.h"

#include <algorithm>

using namespace DeltaEngine;

void EditorSelectionState::BroadcastSelectionChanged()
{
    OnSelectionChanged.Broadcast();
}

bool EditorSelectionState::ClearGameObjectSelectionInternal()
{
    if (m_selectedGameObjects.empty())
        return false;

    m_selectedGameObjects.clear();
    return true;
}

bool EditorSelectionState::ClearComponentSelectionInternal()
{
    if (m_selectedComponents.empty())
        return false;

    m_selectedComponents.clear();
    return true;
}

bool EditorSelectionState::ClearAssetSelectionInternal()
{
    if (m_selectedAssets.empty())
        return false;

    m_selectedAssets.clear();
    return true;
}

bool EditorSelectionState::ClearFolderSelectionInternal()
{
    if (m_selectedFolder.empty())
        return false;

    m_selectedFolder.clear();
    return true;
}

bool EditorSelectionState::RemoveSelectedGameObjectInternal(ObjectId id)
{
    const auto before = m_selectedGameObjects.size();
    m_selectedGameObjects.erase(
        std::remove(m_selectedGameObjects.begin(), m_selectedGameObjects.end(), id),
        m_selectedGameObjects.end());
    return m_selectedGameObjects.size() != before;
}

bool EditorSelectionState::RemoveSelectedComponentInternal(ObjectId id)
{
    const auto before = m_selectedComponents.size();
    m_selectedComponents.erase(
        std::remove(m_selectedComponents.begin(), m_selectedComponents.end(), id),
        m_selectedComponents.end());
    return m_selectedComponents.size() != before;
}

bool EditorSelectionState::RemoveSelectedAssetInternal(AssetId id)
{
    const auto before = m_selectedAssets.size();
    m_selectedAssets.erase(
        std::remove(m_selectedAssets.begin(), m_selectedAssets.end(), id),
        m_selectedAssets.end());
    return m_selectedAssets.size() != before;
}

// --- GameObject selection ---

void EditorSelectionState::SetSelectedGameObject(ObjectId id)
{
    DELTA_CHECK(!id.IsNull());

    bool changed = false;
    changed |= ClearAssetSelectionInternal();
    changed |= ClearFolderSelectionInternal();
    changed |= ClearComponentSelectionInternal();

    if (m_selectedGameObjects.size() != 1 || m_selectedGameObjects[0] != id)
    {
        m_selectedGameObjects.clear();
        m_selectedGameObjects.push_back(id);
        changed = true;
    }

    if (changed)
        BroadcastSelectionChanged();
}

void EditorSelectionState::AddSelectedGameObject(ObjectId id)
{
    DELTA_CHECK(!id.IsNull());

    bool changed = false;
    changed |= ClearAssetSelectionInternal();
    changed |= ClearFolderSelectionInternal();
    changed |= ClearComponentSelectionInternal();

    if (std::find(m_selectedGameObjects.begin(), m_selectedGameObjects.end(), id) == m_selectedGameObjects.end())
    {
        m_selectedGameObjects.push_back(id);
        changed = true;
    }

    if (changed)
        BroadcastSelectionChanged();
}

void EditorSelectionState::RemoveSelectedGameObject(ObjectId id)
{
    if (RemoveSelectedGameObjectInternal(id))
        BroadcastSelectionChanged();
}

void EditorSelectionState::ClearGameObjectSelection()
{
    if (ClearGameObjectSelectionInternal())
        BroadcastSelectionChanged();
}

bool EditorSelectionState::IsGameObjectSelected(ObjectId id) const
{
    return std::find(m_selectedGameObjects.begin(), m_selectedGameObjects.end(), id) != m_selectedGameObjects.end();
}

// --- Component selection ---

void EditorSelectionState::SetSelectedComponent(ObjectId id)
{
    DELTA_CHECK(!id.IsNull());

    bool changed = false;
    changed |= ClearAssetSelectionInternal();
    changed |= ClearFolderSelectionInternal();

    if (m_selectedComponents.size() != 1 || m_selectedComponents[0] != id)
    {
        m_selectedComponents.clear();
        m_selectedComponents.push_back(id);
        changed = true;
    }

    if (changed)
        BroadcastSelectionChanged();
}

void EditorSelectionState::AddSelectedComponent(ObjectId id)
{
    DELTA_CHECK(!id.IsNull());

    bool changed = false;
    changed |= ClearAssetSelectionInternal();
    changed |= ClearFolderSelectionInternal();

    if (std::find(m_selectedComponents.begin(), m_selectedComponents.end(), id) == m_selectedComponents.end())
    {
        m_selectedComponents.push_back(id);
        changed = true;
    }

    if (changed)
        BroadcastSelectionChanged();
}

void EditorSelectionState::RemoveSelectedComponent(ObjectId id)
{
    if (RemoveSelectedComponentInternal(id))
        BroadcastSelectionChanged();
}

void EditorSelectionState::ClearComponentSelection()
{
    if (ClearComponentSelectionInternal())
        BroadcastSelectionChanged();
}

bool EditorSelectionState::IsComponentSelected(ObjectId id) const
{
    return std::find(m_selectedComponents.begin(), m_selectedComponents.end(), id) != m_selectedComponents.end();
}

// --- Asset selection ---

void EditorSelectionState::SetSelectedAsset(AssetId id)
{
    DELTA_CHECK(!id.IsNull());

    bool changed = false;
    changed |= ClearGameObjectSelectionInternal();
    changed |= ClearComponentSelectionInternal();
    changed |= ClearFolderSelectionInternal();

    if (m_selectedAssets.size() != 1 || m_selectedAssets[0] != id)
    {
        m_selectedAssets.clear();
        m_selectedAssets.push_back(id);
        changed = true;
    }

    if (changed)
        BroadcastSelectionChanged();
}

void EditorSelectionState::AddSelectedAsset(AssetId id)
{
    DELTA_CHECK(!id.IsNull());

    bool changed = false;
    changed |= ClearGameObjectSelectionInternal();
    changed |= ClearComponentSelectionInternal();
    changed |= ClearFolderSelectionInternal();

    if (std::find(m_selectedAssets.begin(), m_selectedAssets.end(), id) == m_selectedAssets.end())
    {
        m_selectedAssets.push_back(id);
        changed = true;
    }

    if (changed)
        BroadcastSelectionChanged();
}

void EditorSelectionState::RemoveSelectedAsset(AssetId id)
{
    if (RemoveSelectedAssetInternal(id))
        BroadcastSelectionChanged();
}

void EditorSelectionState::ClearAssetSelection()
{
    if (ClearAssetSelectionInternal())
        BroadcastSelectionChanged();
}

bool EditorSelectionState::IsAssetSelected(AssetId id) const
{
    return std::find(m_selectedAssets.begin(), m_selectedAssets.end(), id) != m_selectedAssets.end();
}

// --- Folder selection ---

void EditorSelectionState::SetSelectedFolder(const std::string& relativePath)
{
    bool changed = false;
    changed |= ClearGameObjectSelectionInternal();
    changed |= ClearComponentSelectionInternal();
    changed |= ClearAssetSelectionInternal();

    if (m_selectedFolder != relativePath)
    {
        m_selectedFolder = relativePath;
        changed = true;
    }

    if (changed)
        BroadcastSelectionChanged();
}

void EditorSelectionState::ClearFolderSelection()
{
    if (ClearFolderSelectionInternal())
        BroadcastSelectionChanged();
}

bool EditorSelectionState::IsFolderSelected(const std::string& relativePath) const
{
    return m_selectedFolder == relativePath;
}

// --- Helpers ---

void EditorSelectionState::ClearAll()
{
    bool changed = false;
    changed |= ClearGameObjectSelectionInternal();
    changed |= ClearComponentSelectionInternal();
    changed |= ClearAssetSelectionInternal();
    changed |= ClearFolderSelectionInternal();

    if (changed)
        BroadcastSelectionChanged();
}

GameObject* EditorSelectionState::GetContextGameObject(EditorCore& core)
{
    if (m_selectedGameObjects.empty())
        return nullptr;

    DPrimaryAsset* asset = core.GetActiveSceneAsset();
    if (!asset)
        return nullptr;

    return dynamic_cast<GameObject*>(asset->FindObject(m_selectedGameObjects[0]));
}

void EditorSelectionState::NotifyObjectDestroyed(const ObjectId& objectId)
{
    if (objectId.IsNull())
        return;

    bool changed = RemoveSelectedGameObjectInternal(objectId);
    changed |= RemoveSelectedComponentInternal(objectId);

    if (changed)
        BroadcastSelectionChanged();
}
