#pragma once

#include "EngineIncludes.h"

#include <string>
#include <vector>
#include <wtypes.h>

DELTA_ENGINE_NS_BEGIN

class IOManager
{
public:
	//static bool readFileToBuffer(const std::string &filePath, std::vector<char>& buffer);
	//static std::wstring GetAssetFullPath(LPCWSTR assetName);
	//static std::string GetAssetFullPath(const std::string & assetName);
    DELTAENGINE_API static std::wstring GetEngineSourceAssetFullPath(std::wstring assetName);
    DELTAENGINE_API static std::string GetEditorSourceAssetFullPath(std::string assetName);
    DELTAENGINE_API static std::string GetEngineImportedAssetsFolder();
    DELTAENGINE_API static std::string GetEngineImportedAssetFullPath(std::string assetName, bool isJson = true);
    DELTAENGINE_API static std::string GetIntermediateFolder();
    DELTAENGINE_API static std::string GetToolsFolder();
};

DELTA_ENGINE_NS_END
