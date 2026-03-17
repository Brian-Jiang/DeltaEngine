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
    static std::wstring GetEngineSourceAssetFullPath(std::wstring assetName);
    DELTAENGINE_API static std::string GetEditorSourceAssetFullPath(std::string assetName);
};

DELTA_ENGINE_NS_END