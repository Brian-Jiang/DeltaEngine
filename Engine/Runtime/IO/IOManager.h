#pragma once

#include "EngineIncludes.h"

#include <filesystem>

DELTA_ENGINE_NS_BEGIN

class IOManager
{
public:
    /** Absolute path to the repository root, resolved once from the executable directory. */
    DELTAENGINE_API static const std::filesystem::path& GetProjectRoot();

    DELTAENGINE_API static std::filesystem::path GetEngineSourceAssetFullPath(const std::filesystem::path& assetName);
    DELTAENGINE_API static std::filesystem::path GetEditorSourceAssetFullPath(const std::filesystem::path& assetName);
    DELTAENGINE_API static std::filesystem::path GetEngineImportedAssetsFolder();
    DELTAENGINE_API static std::filesystem::path GetEngineImportedAssetFullPath(const std::filesystem::path& assetName, bool isJson = true);
    DELTAENGINE_API static std::filesystem::path GetIntermediateFolder();
    DELTAENGINE_API static std::filesystem::path GetToolsFolder();
    DELTAENGINE_API static std::filesystem::path GetSettingsFolder();
    DELTAENGINE_API static std::filesystem::path GetEngineSettingsPath();

    /** Default: Intermediate/EditorState. Tests may override to an isolated folder. */
    DELTAENGINE_API static std::filesystem::path GetEditorStateFolder();
    DELTAENGINE_API static void SetEditorStateFolderOverride(std::filesystem::path path);
    DELTAENGINE_API static void ClearEditorStateFolderOverride();
};

DELTA_ENGINE_NS_END
