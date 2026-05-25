#include "Editor/EditorSelectionState.h"

#include "Editor/EditorCore.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/GameObject.h"

#include <algorithm>

using namespace DeltaEngine;

// --- GameObject selection ---

void EditorSelectionState::SetSelectedGameObject(ObjectId id)
{
    DELTA_CHECK(!id.IsNull());
    ClearAssetSelection();
    ClearFolderSelection();
    m_selectedGameObjects.clear();
    m_selectedGameObjects.push_back(id);
}

void EditorSelectionState::AddSelectedGameObject(ObjectId id)
{
    DELTA_CHECK(!id.IsNull());
    ClearAssetSelection();
    ClearFolderSelection();
    if (std::find(m_selectedGameObjects.begin(), m_selectedGameObjects.end(), id) == m_selectedGameObjects.end())
        m_selectedGameObjects.push_back(id);
}

void EditorSelectionState::RemoveSelectedGameObject(ObjectId id)
{
    m_selectedGameObjects.erase(
        std::remove(m_selectedGameObjects.begin(), m_selectedGameObjects.end(), id),
        m_selectedGameObjects.end());
}

void EditorSelectionState::ClearGameObjectSelection()
{
    m_selectedGameObjects.clear();
}

bool EditorSelectionState::IsGameObjectSelected(ObjectId id) const
{
    return std::find(m_selectedGameObjects.begin(), m_selectedGameObjects.end(), id) != m_selectedGameObjects.end();
}

// --- Component selection ---

void EditorSelectionState::SetSelectedComponent(ObjectId id)
{
    DELTA_CHECK(!id.IsNull());
    ClearAssetSelection();
    ClearFolderSelection();
    m_selectedComponents.clear();
    m_selectedComponents.push_back(id);
}

void EditorSelectionState::AddSelectedComponent(ObjectId id)
{
    DELTA_CHECK(!id.IsNull());
    ClearAssetSelection();
    ClearFolderSelection();
    if (std::find(m_selectedComponents.begin(), m_selectedComponents.end(), id) == m_selectedComponents.end())
        m_selectedComponents.push_back(id);
}

void EditorSelectionState::RemoveSelectedComponent(ObjectId id)
{
    m_selectedComponents.erase(
        std::remove(m_selectedComponents.begin(), m_selectedComponents.end(), id),
        m_selectedComponents.end());
}

void EditorSelectionState::ClearComponentSelection()
{
    m_selectedComponents.clear();
}

bool EditorSelectionState::IsComponentSelected(ObjectId id) const
{
    return std::find(m_selectedComponents.begin(), m_selectedComponents.end(), id) != m_selectedComponents.end();
}

// --- Asset selection ---

void EditorSelectionState::SetSelectedAsset(AssetId id)
{
    DELTA_CHECK(!id.IsNull());
    ClearGameObjectSelection();
    ClearComponentSelection();
    ClearFolderSelection();
    m_selectedAssets.clear();
    m_selectedAssets.push_back(id);
}

void EditorSelectionState::AddSelectedAsset(AssetId id)
{
    DELTA_CHECK(!id.IsNull());
    ClearGameObjectSelection();
    ClearComponentSelection();
    ClearFolderSelection();
    if (std::find(m_selectedAssets.begin(), m_selectedAssets.end(), id) == m_selectedAssets.end())
        m_selectedAssets.push_back(id);
}

void EditorSelectionState::RemoveSelectedAsset(AssetId id)
{
    m_selectedAssets.erase(
        std::remove(m_selectedAssets.begin(), m_selectedAssets.end(), id),
        m_selectedAssets.end());
}

void EditorSelectionState::ClearAssetSelection()
{
    m_selectedAssets.clear();
}

bool EditorSelectionState::IsAssetSelected(AssetId id) const
{
    return std::find(m_selectedAssets.begin(), m_selectedAssets.end(), id) != m_selectedAssets.end();
}

// --- Folder selection ---

void EditorSelectionState::SetSelectedFolder(const std::string& relativePath)
{
    ClearGameObjectSelection();
    ClearComponentSelection();
    ClearAssetSelection();
    m_selectedFolder = relativePath;
}

void EditorSelectionState::ClearFolderSelection()
{
    m_selectedFolder.clear();
}

bool EditorSelectionState::IsFolderSelected(const std::string& relativePath) const
{
    return m_selectedFolder == relativePath;
}

// --- Helpers ---

void EditorSelectionState::ClearAll()
{
    ClearGameObjectSelection();
    ClearComponentSelection();
    ClearAssetSelection();
    ClearFolderSelection();
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

    RemoveSelectedGameObject(objectId);
    RemoveSelectedComponent(objectId);
}
