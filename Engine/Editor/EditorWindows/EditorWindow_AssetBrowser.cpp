#include "Editor/EditorWindows/EditorWindow_AssetBrowser.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/EditorMain.h"
#include "Editor/EditorSelectionState.h"
#include "Runtime/IO/IOManager.h"

#include "imgui.h"

using namespace DeltaEngine;

namespace
{
std::string GetAssetDisplayName(const std::filesystem::path& path)
{
    return path.stem().stem().string();
}
}

EditorWindow_AssetBrowser::EditorWindow_AssetBrowser() = default;
EditorWindow_AssetBrowser::~EditorWindow_AssetBrowser() = default;

void EditorWindow_AssetBrowser::Render()
{
    if (!ImGui::Begin(m_title, m_open))
    {
        ImGui::End();
        return;
    }

    if (!g_editor)
    {
        ImGui::TextDisabled("No editor");
        ImGui::End();
        return;
    }

    EditorAssetDatabase* assetDatabase = g_editor->GetAssetDatabase();
    if (!assetDatabase)
    {
        ImGui::TextDisabled("No asset database");
        ImGui::End();
        return;
    }

    const auto assets = assetDatabase->GetAllAssets();
    if (assets.empty())
    {
        ImGui::TextDisabled("No imported assets");
        ImGui::End();
        return;
    }

    FolderNode rootNode;
    const std::filesystem::path assetRoot(IOManager::GetEngineImportedAssetsFolder());
    BuildTree(rootNode, assets, assetRoot);

    for (const auto& [folderName, childNode] : rootNode.m_children)
        RenderFolderNode(childNode, folderName, folderName, assetDatabase);

    for (const AssetId& assetId : rootNode.m_assets)
        RenderAssetLeaf(assetId, assetDatabase);

    ImGui::End();
}

void EditorWindow_AssetBrowser::BuildTree(FolderNode& root,
    const std::unordered_map<AssetId, EditorAssetDatabase::AssetEntry>& assets,
    const std::filesystem::path& assetRoot) const
{
    for (const auto& [assetId, entry] : assets)
    {
        if (!entry.m_filePath.string().ends_with(".dasset.json"))
            continue;

        std::filesystem::path relativePath;
        try
        {
            relativePath = std::filesystem::relative(entry.m_filePath, assetRoot);
        }
        catch (...)
        {
            relativePath = entry.m_filePath.filename();
        }

        FolderNode* currentNode = &root;
        const std::filesystem::path parentPath = relativePath.parent_path();
        for (const auto& part : parentPath)
            currentNode = &currentNode->m_children[part.string()];

        currentNode->m_assets.push_back(assetId);
    }
}

void EditorWindow_AssetBrowser::RenderFolderNode(const FolderNode& node,
    const std::string& folderName,
    const std::string& fullPath,
    EditorAssetDatabase* assetDatabase)
{
    ImGui::PushID(fullPath.c_str());
    if (ImGui::TreeNodeEx(folderName.c_str(), ImGuiTreeNodeFlags_SpanFullWidth))
    {
        for (const auto& [childName, childNode] : node.m_children)
            RenderFolderNode(childNode, childName, fullPath + "/" + childName, assetDatabase);

        for (const AssetId& assetId : node.m_assets)
            RenderAssetLeaf(assetId, assetDatabase);

        ImGui::TreePop();
    }
    ImGui::PopID();
}

void EditorWindow_AssetBrowser::RenderAssetLeaf(const AssetId& assetId, EditorAssetDatabase* assetDatabase)
{
    if (!assetDatabase || !g_editor)
        return;

    const std::filesystem::path assetPath = assetDatabase->GetAssetPath(assetId);
    if (assetPath.empty())
        return;

    const bool isSelected = g_editor->GetSelectionState() &&
        g_editor->GetSelectionState()->GetSelectedAssetId() == assetId;

    ImGui::PushID(assetPath.generic_string().c_str());
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
        ImGuiTreeNodeFlags_SpanFullWidth;
    if (isSelected)
        flags |= ImGuiTreeNodeFlags_Selected;

    ImGui::TreeNodeEx(GetAssetDisplayName(assetPath).c_str(), flags);
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
    {
        if (assetDatabase->LoadAsset(assetId))
            g_editor->GetSelectionState()->SelectAsset(assetId);
    }

    if (ImGui::BeginPopupContextItem())
    {
        m_assetContextMenu.Open({
            { "Duplicate", [&]()
                {
                    const AssetId duplicatedId = assetDatabase->DuplicateAsset(assetId);
                    if (!duplicatedId.IsNull() && assetDatabase->LoadAsset(duplicatedId))
                        g_editor->GetSelectionState()->SelectAsset(duplicatedId);
                } },
            { "Delete", [&]()
                {
                    const bool wasSelected = g_editor->GetSelectionState()->GetSelectedAssetId() == assetId;
                    assetDatabase->DeleteAsset(assetId);
                    if (wasSelected)
                        g_editor->GetSelectionState()->ClearSelection();
                } },
        });
        m_assetContextMenu.Draw(g_editor->GetEditorTheme()->colors);
        ImGui::EndPopup();
    }

    ImGui::PopID();
}
