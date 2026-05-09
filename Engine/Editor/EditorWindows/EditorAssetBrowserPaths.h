#pragma once

#include "EditorIncludes.h"

#include <filesystem>
#include <string>

DELTA_ENGINE_NS_BEGIN

DELTAEDITOR_API std::string GetAssetDisplayNameForBrowser(const std::filesystem::path& path);

DELTAEDITOR_API std::filesystem::path UniqueDirUnderParent(const std::filesystem::path& parent, const std::string& base);

DELTAEDITOR_API std::filesystem::path UniqueAssetPathInFolder(const std::filesystem::path& folder, const std::string& base);

DELTAEDITOR_API std::filesystem::path NormalizeEditorPath(const std::filesystem::path& path);

DELTAEDITOR_API bool IsSameOrChildPathNormalized(const std::filesystem::path& path, const std::filesystem::path& parent);

DELTAEDITOR_API std::string ToAssetRootRelativeString(const std::filesystem::path& path, const std::filesystem::path& assetRoot);

DELTA_ENGINE_NS_END
