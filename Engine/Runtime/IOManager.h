#pragma once

#include <string>
#include <vector>
#include <winnt.h>

class IOManager {
public:
	static bool readFileToBuffer(const std::string &filePath, std::vector<char>& buffer);
	static std::wstring GetAssetFullPath(LPCWSTR assetName);
	static std::string GetAssetFullPath(const std::string & assetName);
};
