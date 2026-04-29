#pragma once

#include "UIComponents/ContextMenuPopup.h"
#include "UIComponents/EditorInlineRename.h"

#include "Assets/EditorAssetDatabase.h"
#include "EditorWindows/EditorWindow.h"

#include "Runtime/Core/UUID.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class EditorAssetDatabase;

class EditorWindow_AssetBrowser : public EditorWindow
{
public:
    EditorWindow_AssetBrowser();
    ~EditorWindow_AssetBrowser();

    /// Folder tree of imported assets with select / duplicate / delete actions.
    void Render(bool& open) override;

private:
    struct FolderNode
    {
        std::map<std::string, FolderNode> m_children;
        std::vector<AssetId>              m_assets;
    };

    void BuildTree(FolderNode& root, const std::unordered_map<AssetId, EditorAssetDatabase::AssetEntry>& assets,
        const std::filesystem::path& assetRoot) const;
    void RenderFolderNode(const FolderNode& node, const std::string& folderName,
        const std::string& fullPath, EditorAssetDatabase* assetDatabase);
    void RenderAssetLeaf(const AssetId& assetId, EditorAssetDatabase* assetDatabase);
    void RenderImportButton(EditorAssetDatabase* assetDatabase);
    void RenderCreateButton(EditorAssetDatabase* assetDatabase);

    std::filesystem::path GetTargetFolder(EditorAssetDatabase* assetDatabase) const;

    ContextMenuPopup   m_assetContextMenu;
    EditorInlineRename m_inlineRename;
    AssetId            m_renameAssetId       = AssetId::Null();
    std::string           m_renameFolderRelPath;
    std::filesystem::path m_renameFolderAbsPath;
    AssetId               m_scrollToAssetId  = AssetId::Null();
    std::string           m_scrollToFolderRelPath;
};

DELTA_ENGINE_NS_END
