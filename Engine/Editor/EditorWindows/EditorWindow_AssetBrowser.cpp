#include "Editor/EditorWindows/EditorWindow_AssetBrowser.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/Commands/EditorAuxiliarySceneCommands.h"
#include "Editor/Commands/EditorCommand_RenameAsset.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/EditorCore.h"
#include "Editor/EditorMain.h"
#include "Editor/EditorSelectionState.h"
#include "Editor/EditorWindows/EditorAssetBrowserPaths.h"
#include "Editor/EditorWindows/EditorWindowsLog.h"
#include "Runtime/Assets/PA_CommonAssets.h"
#include "Runtime/Assets/PA_DScene.h"
#include "Runtime/Core/Skybox.h"
#include "Runtime/Graphics/PostProcess/PA_PostProcessStack.h"
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
    ofn.Flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

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
    ImGui::SameLine();
    RenderCreateButton(assetDatabase);
    ImGui::Separator();

    const uint64_t revision = assetDatabase->GetAssetSetRevision();
    if (revision != m_cachedRevision)
    {
        m_cachedTree = FolderNode{};
        const std::filesystem::path assetRoot(IOManager::GetEngineImportedAssetsFolder());
        BuildTree(m_cachedTree, assetDatabase->GetAllAssets(), assetRoot);
        m_cachedRevision = revision;
    }

    const FolderNode& rootNode = m_cachedTree;
    if (rootNode.m_children.empty() && rootNode.m_assets.empty())
    {
        ImGui::TextDisabled("No imported assets");
        ImGui::End();
        return;
    }

    const std::filesystem::path assetRoot(IOManager::GetEngineImportedAssetsFolder());

    constexpr ImGuiTreeNodeFlags kRootFlags =
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth;
    if (ImGui::TreeNodeEx("EngineImportedAssets", kRootFlags))
    {
        if (ImGui::BeginDragDropTarget())
        {
            if (ImGui::AcceptDragDropPayload("ASSET_MOVE"))
                MoveSelectedItems(assetDatabase, assetRoot);
            ImGui::EndDragDropTarget();
        }

        for (const auto& [folderName, childNode] : rootNode.m_children)
            RenderFolderNode(childNode, folderName, folderName, assetDatabase);

        for (const AssetId& assetId : rootNode.m_assets)
            RenderAssetLeaf(assetId, assetDatabase);

        ImGui::TreePop();
    }

    if (ImGui::GetDragDropPayload() != nullptr)
    {
        constexpr float kScrollZone  = 30.f;
        constexpr float kScrollSpeed = 5.f;
        const float mouseY    = ImGui::GetMousePos().y;
        const float winTop    = ImGui::GetWindowPos().y;
        const float winBottom = winTop + ImGui::GetWindowHeight();
        if (mouseY < winTop + kScrollZone)
            ImGui::SetScrollY(ImGui::GetScrollY() - kScrollSpeed);
        else if (mouseY > winBottom - kScrollZone)
            ImGui::SetScrollY(ImGui::GetScrollY() + kScrollSpeed);
    }

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
                m_inlineRename.Begin(GetAssetDisplayNameForBrowser(p));
            }
        }
        else if (sel && sel->HasFolderSelection())
        {
            m_renameFolderRelPath = sel->GetSelectedFolder();
            m_renameFolderAbsPath =
                NormalizeEditorPath(std::filesystem::path(IOManager::GetEngineImportedAssetsFolder()) / m_renameFolderRelPath);
            m_inlineRename.Begin(m_renameFolderAbsPath.filename().string());
        }
    }

    ImGui::End();
}

void EditorWindow_AssetBrowser::BuildTree(FolderNode& root,
    const std::unordered_map<AssetId, EditorAssetDatabase::AssetEntry>& assets,
    const std::filesystem::path& assetRoot) const
{
    try
    {
        for (const auto& dirEntry : std::filesystem::recursive_directory_iterator(
                 assetRoot, std::filesystem::directory_options::skip_permission_denied))
        {
            if (!dirEntry.is_directory())
                continue;
            const auto rel = dirEntry.path().lexically_relative(assetRoot);
            FolderNode* cur = &root;
            for (const auto& part : rel)
                cur = &cur->m_children[part.string()];
        }
    }
    catch (const std::exception& ex)
    {
        DLOG(LogEditorWindows, ELogLevel::Warning,
            "Asset browser directory scan failed under '{}': {} (expected readable asset root tree)",
            assetRoot.string(), ex.what());
    }
    catch (...)
    {
        DLOG(LogEditorWindows, ELogLevel::Warning,
            "Asset browser directory scan failed under '{}' with unknown exception (expected readable asset root tree)",
            assetRoot.string());
    }

    for (const auto& [assetId, entry] : assets)
    {
        if (!entry.m_filePath.string().ends_with(".dasset.json"))
            continue;

        std::filesystem::path relativePath;
        try
        {
            relativePath = entry.m_filePath.lexically_relative(assetRoot);
        }
        catch (const std::exception& ex)
        {
            DLOG(LogEditorWindows, ELogLevel::Warning,
                "relative() failed for asset path '{}' versus root '{}': {} (fallback to filename only)",
                entry.m_filePath.string(), assetRoot.string(), ex.what());
            relativePath = entry.m_filePath.filename();
        }
        catch (...)
        {
            DLOG(LogEditorWindows, ELogLevel::Warning,
                "relative() failed for asset path '{}' versus root '{}' with unknown exception (fallback to filename only)",
                entry.m_filePath.string(), assetRoot.string());
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
    const bool renamingThis = m_inlineRename.IsActive() && fullPath == m_renameFolderRelPath;
    EditorSelectionState* sel = g_editorCore ? g_editorCore->GetSelectionState() : nullptr;
    const bool isSelected = sel && sel->IsFolderSelected(fullPath);

    if (!m_scrollToFolderRelPath.empty() && fullPath == m_scrollToFolderRelPath)
    {
        ImGui::SetScrollHereY();
        m_scrollToFolderRelPath.clear();
    }

    ImGui::PushID(fullPath.c_str());

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow;
    if (isSelected)
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    if (renamingThis)
        nodeFlags |= ImGuiTreeNodeFlags_AllowOverlap;

    const bool folderOpen = ImGui::TreeNodeEx(folderName.c_str(), nodeFlags);

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        if (sel && !sel->IsFolderSelected(fullPath))
            sel->SetSelectedFolder(fullPath);
        const int dummy = 0;
        ImGui::SetDragDropPayload("ASSET_MOVE", &dummy, sizeof(dummy));
        ImGui::Text("Moving folder");
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (ImGui::AcceptDragDropPayload("ASSET_MOVE"))
        {
            const std::filesystem::path absTarget =
                std::filesystem::absolute(std::filesystem::path(IOManager::GetEngineImportedAssetsFolder()) / fullPath);
            MoveSelectedItems(assetDatabase, absTarget);
        }
        ImGui::EndDragDropTarget();
    }

    if (!renamingThis && ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen() && sel)
        sel->SetSelectedFolder(fullPath);

    if (ImGui::BeginPopupContextItem())
    {
        if (sel)
            sel->SetSelectedFolder(fullPath);

        std::vector<ContextMenuPopup::Item> items;
        items.push_back({"Rename", [&, fullPath]() {
            m_renameFolderRelPath = fullPath;
            m_renameFolderAbsPath =
                NormalizeEditorPath(std::filesystem::path(IOManager::GetEngineImportedAssetsFolder()) / fullPath);
            m_inlineRename.Begin(m_renameFolderAbsPath.filename().string());
        }});
        items.push_back({"Delete", [&, fullPath]() {
            DeleteFolder(assetDatabase, fullPath);
        }});
        m_assetContextMenu.Open(std::move(items));
        m_assetContextMenu.Draw(g_editor->GetEditorTheme()->colors);
        ImGui::EndPopup();
    }

    if (folderOpen)
    {
        if (renamingThis)
        {
            ImGui::SameLine(ImGui::GetCursorPosX());
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 8.f);
            const auto rr = m_inlineRename.Draw();
            if (rr == EditorInlineRename::Result::Committed)
            {
                RenameFolder(assetDatabase, m_renameFolderRelPath, m_inlineRename.GetBuffer());
                m_renameFolderRelPath.clear();
                m_renameFolderAbsPath.clear();
            }
            else if (rr == EditorInlineRename::Result::Cancelled)
            {
                m_renameFolderRelPath.clear();
                m_renameFolderAbsPath.clear();
            }
        }

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
    if (!sel)
        return;
    const bool isSelected = sel->IsAssetSelected(assetId);

    ImGui::PushID(assetPath.generic_string().c_str());
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
        ImGuiTreeNodeFlags_SpanFullWidth;
    if (isSelected)
        flags |= ImGuiTreeNodeFlags_Selected;

    const bool renamingRow = m_inlineRename.IsActive() && assetId == m_renameAssetId;
    if (renamingRow)
        flags |= ImGuiTreeNodeFlags_AllowOverlap;

    ImGui::TreeNodeEx(GetAssetDisplayNameForBrowser(assetPath).c_str(), flags);

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    {
        if (sel && !sel->IsAssetSelected(assetId))
            sel->SetSelectedAsset(assetId);
        const int dummy = 0;
        ImGui::SetDragDropPayload("ASSET_MOVE", &dummy, sizeof(dummy));
        const std::size_t count = sel ? sel->GetSelectedAssets().size() : 1;
        ImGui::Text("Moving %zu asset(s)", count);
        ImGui::EndDragDropSource();
    }

    if (!m_scrollToAssetId.IsNull() && assetId == m_scrollToAssetId)
    {
        ImGui::SetScrollHereY();
        m_scrollToAssetId = AssetId::Null();
    }

    const bool treeHit = !renamingRow && ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();
    if (treeHit)
    {
        assetDatabase->LoadAsset(assetId);
        if (ImGui::GetIO().KeyCtrl)
        {
            if (sel->IsAssetSelected(assetId))
                sel->RemoveSelectedAsset(assetId);
            else
                sel->AddSelectedAsset(assetId);
        }
        else
        {
            sel->SetSelectedAsset(assetId);
        }
    }

    if (ImGui::BeginPopupContextItem())
    {
        std::vector<ContextMenuPopup::Item> items;
        const bool canRename = sel && sel->GetSelectedAssets().size() == 1 &&
            sel->GetSelectedAssets()[0] == assetId;
        if (canRename)
        {
            items.push_back({"Rename", [&]() {
                m_renameAssetId = assetId;
                m_inlineRename.Begin(GetAssetDisplayNameForBrowser(assetPath));
            }});
        }
        if (const DPrimaryAsset::Header* header = assetDatabase->GetAssetHeader(assetId);
            header && header->m_className == "PA_DScene")
        {
            items.push_back({ "Load Scene", [&]()
                {
                    const std::filesystem::path scenePath = assetDatabase->GetAssetPath(assetId);
                    EditorCommandContext ctx{ *g_editorCore };
                    g_editorCore->GetCommandManager().ExecuteAuxiliary(
                        std::make_unique<EditorAuxiliaryCommand_LoadScene>(scenePath), ctx);
                } });
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

    if (renamingRow)
    {
        ImGui::SameLine(ImGui::GetCursorPosX());
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 8.f);
        const auto rr = m_inlineRename.Draw();
        if (rr == EditorInlineRename::Result::Committed)
        {
            EditorCommandContext ctx{ *g_editorCore };
            g_editorCore->GetCommandManager().Execute(
                std::make_unique<EditorCommand_RenameAsset>(assetId, std::string(m_inlineRename.GetBuffer())),
                ctx);
            m_renameAssetId = AssetId::Null();
        }
        else if (rr == EditorInlineRename::Result::Cancelled)
            m_renameAssetId = AssetId::Null();
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

void EditorWindow_AssetBrowser::MoveSelectedItems(EditorAssetDatabase* assetDatabase,
    const std::filesystem::path& targetFolder)
{
    if (!assetDatabase || !g_editorCore)
    {
        DLOG(LogEditorWindows, ELogLevel::Warning,
            "MoveSelectedItems skipped: assetDatabase or EditorCore missing (expected valid browser context)");
        return;
    }
    EditorSelectionState* sel = g_editorCore->GetSelectionState();
    if (!sel)
    {
        DLOG(LogEditorWindows, ELogLevel::Warning,
            "MoveSelectedItems skipped: selection state missing (expected EditorSelectionState from EditorCore)");
        return;
    }

    if (sel->HasFolderSelection())
    {
        const std::filesystem::path assetRoot = NormalizeEditorPath(IOManager::GetEngineImportedAssetsFolder());
        const std::filesystem::path source    = NormalizeEditorPath(assetRoot / sel->GetSelectedFolder());
        const std::filesystem::path target    = NormalizeEditorPath(targetFolder);
        if (!std::filesystem::exists(source) || !std::filesystem::is_directory(source))
        {
            DLOG(LogEditorWindows, ELogLevel::Warning,
                "Folder move aborted: source '{}' missing or not a directory (expected selected folder under '{}')",
                source.string(), assetRoot.string());
            return;
        }
        if (target == source.parent_path() || IsSameOrChildPathNormalized(target, source))
        {
            DLOG(LogEditorWindows, ELogLevel::Warning,
                "Folder move aborted: invalid target '{}' for source '{}' (expected different parent and not under source subtree)",
                target.string(), source.string());
            return;
        }

        const std::filesystem::path destination = target / source.filename();
        if (std::filesystem::exists(destination))
        {
            DLOG(LogEditorWindows, ELogLevel::Warning,
                "Folder move aborted: destination '{}' already exists (expected unique folder name)",
                destination.string());
            return;
        }

        std::error_code ec;
        std::filesystem::create_directories(destination, ec);
        if (ec)
        {
            DLOG(LogEditorWindows, ELogLevel::Error,
                "Folder move failed: create_directories '{}' error {} (expected writable intermediate path)",
                destination.string(), ec.message());
            return;
        }

        for (const auto& entry : std::filesystem::recursive_directory_iterator(source))
        {
            if (!entry.is_directory())
                continue;
            const auto rel = std::filesystem::relative(entry.path(), source, ec);
            if (!ec)
                std::filesystem::create_directories(destination / rel, ec);
        }

        std::vector<AssetId> containedAssets;
        for (const auto& [assetId, entry] : assetDatabase->GetAllAssets())
        {
            if (IsSameOrChildPathNormalized(entry.m_filePath.parent_path(), source))
                containedAssets.push_back(assetId);
        }

        bool movedAllAssets = true;
        for (const AssetId& id : containedAssets)
        {
            const std::filesystem::path oldPath = assetDatabase->GetAssetPath(id);
            const auto relParent = std::filesystem::relative(oldPath.parent_path(), source, ec);
            if (ec)
            {
                movedAllAssets = false;
                DLOG(LogEditorWindows, ELogLevel::Warning,
                    "Folder move partially failed: relative() asset '{}' vs source '{}' error {}",
                    oldPath.string(), source.string(), ec.message());
                continue;
            }
            const std::filesystem::path newParent = destination / relParent;
            std::filesystem::create_directories(newParent, ec);
            if (!ec)
                movedAllAssets = assetDatabase->MoveAsset(id, newParent) && movedAllAssets;
            else
            {
                movedAllAssets = false;
                DLOG(LogEditorWindows, ELogLevel::Warning,
                    "Folder move failed creating '{}': {}",
                    newParent.string(), ec.message());
            }
        }

        if (!movedAllAssets)
        {
            DLOG(LogEditorWindows, ELogLevel::Error,
                "Folder move aborted for '{}' -> '{}': one or more assets did not move (expected complete move before delete)",
                source.string(), destination.string());
            return;
        }

        std::filesystem::remove_all(source, ec);
        if (ec)
            DLOG(LogEditorWindows, ELogLevel::Error,
                "Folder move incomplete: remove_all '{}' failed: {} (expected empty source folder after asset moves)",
                source.string(), ec.message());
        assetDatabase->BumpAssetSetRevision();
        const std::string newRel =
            ToAssetRootRelativeString(destination, IOManager::GetEngineImportedAssetsFolder());
        if (!newRel.empty())
            sel->SetSelectedFolder(newRel);
        return;
    }

    const std::vector<AssetId> toMove = sel->GetSelectedAssets();
    for (const AssetId& id : toMove)
    {
        if (!assetDatabase->MoveAsset(id, targetFolder))
            DLOG(LogEditorWindows, ELogLevel::Warning,
                "MoveAsset failed for asset id (MoveAsset returned false): target '{}' (expected writable destination)",
                NormalizeEditorPath(targetFolder).string());
    }
}

void EditorWindow_AssetBrowser::RenameFolder(EditorAssetDatabase* assetDatabase,
    const std::string& folderRelPath,
    const std::string& newName)
{
    if (!assetDatabase || !g_editorCore || folderRelPath.empty() || newName.empty())
    {
        DLOG(LogEditorWindows, ELogLevel::Warning,
            "RenameFolder skipped: invalid inputs (assetDatabase={}, editorCore={}, relPath empty={}, newName empty={})",
            static_cast<void*>(assetDatabase), static_cast<void*>(g_editorCore), folderRelPath.empty(), newName.empty());
        return;
    }

    EditorSelectionState* sel = g_editorCore->GetSelectionState();
    if (!sel)
    {
        DLOG(LogEditorWindows, ELogLevel::Warning,
            "RenameFolder skipped: selection state missing");
        return;
    }

    const std::filesystem::path assetRoot = NormalizeEditorPath(IOManager::GetEngineImportedAssetsFolder());
    const std::filesystem::path source    = NormalizeEditorPath(assetRoot / folderRelPath);
    const std::filesystem::path target    = source.parent_path() / newName;
    if (!std::filesystem::exists(source) || std::filesystem::exists(target))
    {
        DLOG(LogEditorWindows, ELogLevel::Warning,
            "RenameFolder aborted: source exists={}, target exists={}, source='{}', target='{}' (expected existing source and free target name)",
            std::filesystem::exists(source), std::filesystem::exists(target), source.string(), target.string());
        return;
    }

    if (source.filename() == newName)
        return;

    std::error_code ec;
    std::filesystem::create_directories(target, ec);
    if (ec)
    {
        DLOG(LogEditorWindows, ELogLevel::Error,
            "RenameFolder failed: create_directories '{}' error {} (expected writable path)",
            target.string(), ec.message());
        return;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(source))
    {
        if (!entry.is_directory())
            continue;
        const auto rel = std::filesystem::relative(entry.path(), source, ec);
        if (!ec)
            std::filesystem::create_directories(target / rel, ec);
    }

    std::vector<AssetId> containedAssets;
    for (const auto& [assetId, entry] : assetDatabase->GetAllAssets())
    {
        if (IsSameOrChildPathNormalized(entry.m_filePath.parent_path(), source))
            containedAssets.push_back(assetId);
    }

    bool movedAllAssets = true;
    for (const AssetId& id : containedAssets)
    {
        const std::filesystem::path oldPath = assetDatabase->GetAssetPath(id);
        const auto relParent = std::filesystem::relative(oldPath.parent_path(), source, ec);
        if (ec)
        {
            movedAllAssets = false;
            DLOG(LogEditorWindows, ELogLevel::Warning,
                "RenameFolder: relative failed for '{}' vs '{}': {}",
                oldPath.string(), source.string(), ec.message());
            continue;
        }
        const std::filesystem::path newParent = target / relParent;
        std::filesystem::create_directories(newParent, ec);
        if (!ec)
            movedAllAssets = assetDatabase->MoveAsset(id, newParent) && movedAllAssets;
        else
        {
            movedAllAssets = false;
            DLOG(LogEditorWindows, ELogLevel::Warning,
                "RenameFolder: mkdir '{}' failed: {}", newParent.string(), ec.message());
        }
    }

    if (!movedAllAssets)
    {
        DLOG(LogEditorWindows, ELogLevel::Error,
            "RenameFolder aborted for '{}' -> '{}': asset registry move incomplete",
            source.string(), target.string());
        return;
    }

    std::filesystem::remove_all(source, ec);
    if (ec)
        DLOG(LogEditorWindows, ELogLevel::Error,
            "RenameFolder: remove_all '{}' failed: {} after moving assets",
            source.string(), ec.message());
    assetDatabase->BumpAssetSetRevision();
    const std::string newRel = ToAssetRootRelativeString(target, IOManager::GetEngineImportedAssetsFolder());
    if (!newRel.empty())
        sel->SetSelectedFolder(newRel);
}

void EditorWindow_AssetBrowser::DeleteFolder(EditorAssetDatabase* assetDatabase, const std::string& folderRelPath)
{
    if (!assetDatabase || !g_editorCore || folderRelPath.empty())
    {
        DLOG(LogEditorWindows, ELogLevel::Warning,
            "DeleteFolder skipped: invalid arguments (folderRelPath empty or missing context)");
        return;
    }

    const std::filesystem::path folderPath =
        NormalizeEditorPath(std::filesystem::path(IOManager::GetEngineImportedAssetsFolder()) / folderRelPath);

    std::vector<AssetId> containedAssets;
    for (const auto& [assetId, entry] : assetDatabase->GetAllAssets())
    {
        if (IsSameOrChildPathNormalized(entry.m_filePath.parent_path(), folderPath))
            containedAssets.push_back(assetId);
    }

    for (const AssetId& id : containedAssets)
        assetDatabase->DeleteAsset(id);

    std::error_code ec;
    std::filesystem::remove_all(folderPath, ec);
    if (ec)
        DLOG(LogEditorWindows, ELogLevel::Error,
            "DeleteFolder: remove_all '{}' failed: {} (expected deletable folder after asset purge)",
            folderPath.string(), ec.message());
    assetDatabase->BumpAssetSetRevision();

    EditorSelectionState* sel = g_editorCore->GetSelectionState();
    if (sel && sel->IsFolderSelected(folderRelPath))
        sel->ClearFolderSelection();
}

std::filesystem::path EditorWindow_AssetBrowser::GetTargetFolder(EditorAssetDatabase* assetDatabase) const
{
    const std::filesystem::path root = IOManager::GetEngineImportedAssetsFolder();
    EditorSelectionState* sel        = g_editorCore ? g_editorCore->GetSelectionState() : nullptr;
    if (sel && sel->HasFolderSelection())
        return root / sel->GetSelectedFolder();
    if (sel && !sel->GetSelectedAssets().empty())
    {
        const auto p = assetDatabase->GetAssetPath(sel->GetSelectedAssets()[0]);
        if (!p.empty())
            return p.parent_path();
    }
    return root;
}

void EditorWindow_AssetBrowser::RenderCreateButton(EditorAssetDatabase* assetDatabase)
{
    if (ImGui::Button("Create"))
        ImGui::OpenPopup("##create_popup");

    if (!ImGui::BeginPopup("##create_popup"))
        return;

    const std::filesystem::path assetRoot = IOManager::GetEngineImportedAssetsFolder();

    if (ImGui::MenuItem("New Folder"))
    {
        const std::filesystem::path target = GetTargetFolder(assetDatabase);
        const std::filesystem::path absPath = UniqueDirUnderParent(std::filesystem::absolute(target), "NewFolder");
        std::error_code ec;
        std::filesystem::create_directory(absPath, ec);
        if (!ec)
        {
            const auto relPath =
                std::filesystem::relative(absPath, std::filesystem::absolute(assetRoot)).generic_string();
            m_renameFolderRelPath    = relPath;
            m_renameFolderAbsPath    = absPath;
            m_scrollToFolderRelPath  = relPath;
            m_inlineRename.Begin(absPath.filename().string());
            g_editorCore->GetSelectionState()->SetSelectedFolder(relPath);
            assetDatabase->BumpAssetSetRevision();
        }
        else
            DLOG(LogEditorWindows, ELogLevel::Error,
                "create_directory '{}' failed: {} (expected writable parent under '{}')",
                absPath.string(), ec.message(), assetRoot.string());
    }

    ImGui::Separator();

    auto createAsset = [&](const char* label, const char* defaultName, auto makeAsset)
    {
        if (!ImGui::MenuItem(label))
            return;
        const std::filesystem::path target   = GetTargetFolder(assetDatabase);
        const std::filesystem::path filePath = UniqueAssetPathInFolder(target, defaultName);
        auto* asset                          = makeAsset();
        assetDatabase->CreateAsset(filePath, asset);
        const AssetId newId = assetDatabase->FindAssetIdByPath(filePath);
        if (!newId.IsNull())
        {
            assetDatabase->LoadAsset(newId);
            g_editorCore->GetSelectionState()->SetSelectedAsset(newId);
            m_renameAssetId   = newId;
            m_scrollToAssetId = newId;
            m_inlineRename.Begin(defaultName);
        }
    };

    createAsset("New Scene", "NewDScene", []() -> DPrimaryAsset* {
        return PA_DScene::Create("NewDScene");
    });
    createAsset("New Skybox", "NewSkybox", []() -> DPrimaryAsset* {
        return PA_Skybox::Create(CreateDObject<Skybox>());
    });
    createAsset("New Post-Process Stack", "NewPostProcessStack", []() -> DPrimaryAsset* {
        return PA_PostProcessStack::Create();
    });

    ImGui::EndPopup();
}
