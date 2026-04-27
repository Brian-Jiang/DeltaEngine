#include "Editor/EditorWindows/EditorWindow_AssetBrowser.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/Commands/EditorCommand_RenameAsset.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/EditorCore.h"
#include "Editor/EditorMain.h"
#include "Editor/EditorSelectionState.h"
#include "Runtime/IO/IOManager.h"

#include "imgui.h"

#include <commdlg.h>

#include <filesystem>
#include <vector>

using namespace DeltaEngine;

namespace
{

std::vector<std::filesystem::path> OpenImportFileDialog()
{
    static constexpr DWORD kBufSize = 32768;
    static wchar_t fileBuffer[kBufSize];
    fileBuffer[0] = L'\0';

    OPENFILENAMEW ofn = {};
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = nullptr;
    ofn.lpstrFilter  = L"Supported Assets\0*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.dds;*.hdr;*.fbx;*.obj;*.slang\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFile    = fileBuffer;
    ofn.nMaxFile     = kBufSize;
    ofn.lpstrTitle   = L"Import Assets";
    ofn.Flags        = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    std::vector<std::filesystem::path> result;
    if (!GetOpenFileNameW(&ofn))
        return result;

    const wchar_t* ptr = fileBuffer;
    std::wstring first = ptr;
    ptr += first.size() + 1;

    if (*ptr == L'\0')
    {
        result.emplace_back(first);
    }
    else
    {
        while (*ptr != L'\0')
        {
            std::wstring name = ptr;
            result.emplace_back(std::filesystem::path(first) / name);
            ptr += name.size() + 1;
        }
    }

    return result;
}

} // namespace

namespace
{
std::string GetAssetDisplayName(const std::filesystem::path& path)
{
    return path.stem().stem().string();
}
}

EditorWindow_AssetBrowser::EditorWindow_AssetBrowser()
{
    m_title = "Asset Browser";
}

EditorWindow_AssetBrowser::~EditorWindow_AssetBrowser() = default;

void EditorWindow_AssetBrowser::Render(bool& open)
{
    if (!ImGui::Begin(GetImGuiTitle(), &open))
    {
        ImGui::End();
        return;
    }

    if (!g_editorCore)
    {
        ImGui::TextDisabled("No editor");
        ImGui::End();
        return;
    }

    EditorAssetDatabase* assetDatabase = g_editorCore->GetAssetDatabase();
    if (!assetDatabase)
    {
        ImGui::TextDisabled("No asset database");
        ImGui::End();
        return;
    }

    RenderImportButton(assetDatabase);
    ImGui::Separator();

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

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::GetIO().WantTextInput &&
        ImGui::IsKeyPressed(ImGuiKey_F2))
    {
        EditorSelectionState* sel = g_editorCore->GetSelectionState();
        if (sel && sel->GetSelectedAssets().size() == 1)
        {
            const AssetId id = sel->GetSelectedAssets()[0];
            const std::filesystem::path p = assetDatabase->GetAssetPath(id);
            if (!p.empty())
            {
                m_renameAssetId = id;
                m_inlineRename.Begin(GetAssetDisplayName(p));
            }
        }
    }

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
    if (!assetDatabase || !g_editorCore)
        return;

    const std::filesystem::path assetPath = assetDatabase->GetAssetPath(assetId);
    if (assetPath.empty())
        return;

    EditorSelectionState* sel = g_editorCore->GetSelectionState();
    const bool isSelected     = sel && sel->IsAssetSelected(assetId);

    ImGui::PushID(assetPath.generic_string().c_str());
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
        ImGuiTreeNodeFlags_SpanFullWidth;
    if (isSelected)
        flags |= ImGuiTreeNodeFlags_Selected;

    ImGui::TreeNodeEx(&assetId, flags, "##asset");
    const bool treeHit = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();

    if (ImGui::BeginPopupContextItem())
    {
        std::vector<ContextMenuPopup::Item> items;
        const bool canRename = sel && sel->GetSelectedAssets().size() == 1 &&
            sel->GetSelectedAssets()[0] == assetId;
        if (canRename)
        {
            items.push_back({"Rename", [&]() {
                m_renameAssetId = assetId;
                m_inlineRename.Begin(GetAssetDisplayName(assetPath));
            }});
        }
        items.push_back({ "Duplicate", [&]()
            {
                const AssetId duplicatedId = assetDatabase->DuplicateAsset(assetId);
                if (!duplicatedId.IsNull() && assetDatabase->LoadAsset(duplicatedId))
                    g_editorCore->GetSelectionState()->SetSelectedAsset(duplicatedId);
            } });
        items.push_back({ "Delete", [&]()
            {
                const bool wasSelected = g_editorCore->GetSelectionState()->IsAssetSelected(assetId);
                assetDatabase->DeleteAsset(assetId);
                if (wasSelected)
                    g_editorCore->GetSelectionState()->ClearAssetSelection();
            } });
        m_assetContextMenu.Open(std::move(items));
        m_assetContextMenu.Draw(g_editor->GetEditorTheme()->colors);
        ImGui::EndPopup();
    }

    ImGui::SameLine(0.f, 6.f);

    const bool renamingRow = m_inlineRename.IsActive() && assetId == m_renameAssetId;
    if (renamingRow)
    {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 8.f);
        const auto rr = m_inlineRename.Draw();
        if (rr == EditorInlineRename::Committed)
        {
            EditorCommandContext ctx{ *g_editorCore };
            g_editorCore->GetCommandManager().Execute(
                std::make_unique<EditorCommand_RenameAsset>(assetId, std::string(m_inlineRename.GetBuffer())),
                ctx);
            m_renameAssetId = AssetId::Null();
        }
        else if (rr == EditorInlineRename::Cancelled)
            m_renameAssetId = AssetId::Null();
    }
    else
        ImGui::TextUnformatted(GetAssetDisplayName(assetPath).c_str());

    bool labelHit = false;
    if (!renamingRow)
        labelHit = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();
    if (treeHit || labelHit)
    {
        if (assetDatabase->LoadAsset(assetId))
            g_editorCore->GetSelectionState()->SetSelectedAsset(assetId);
    }

    ImGui::PopID();
}

void EditorWindow_AssetBrowser::RenderImportButton(EditorAssetDatabase* assetDatabase)
{
    if (!ImGui::Button("Import..."))
        return;

    const std::vector<std::filesystem::path> paths = OpenImportFileDialog();
    if (paths.empty())
        return;

    const std::vector<AssetId> imported = assetDatabase->ImportAssets(paths);
    if (!imported.empty())
    {
        if (assetDatabase->LoadAsset(imported.back()))
            g_editorCore->GetSelectionState()->SetSelectedAsset(imported.back());
    }
}
