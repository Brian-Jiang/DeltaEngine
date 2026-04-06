#pragma once

#include "UIComponents/ContextMenuPopup.h"

#include "Assets/EditorAssetDatabase.h"
#include "EditorWindows/EditorWindow.h"

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

    /// ImGui window title.
    const char* m_title = "Asset Browser";

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

    ContextMenuPopup m_assetContextMenu;
};

DELTA_ENGINE_NS_END
