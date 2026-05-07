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
};

DELTA_ENGINE_NS_END
