#pragma once

#include "EditorIncludes.h"

#include "Runtime/Core/UUID.h"

#include <filesystem>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class EditorAssetDatabase;
class DTexture;

class DELTAEDITOR_API AssetImporter
{
public:
    /** Imports a source file into the asset database. Returns all created asset IDs. */
    static std::vector<AssetId> ImportFile(const std::filesystem::path& sourcePath, EditorAssetDatabase& db);

private:
    static AssetId ImportTexture(const std::filesystem::path& sourcePath, const std::filesystem::path& destDir, EditorAssetDatabase& db);
    static AssetId ImportShader(const std::filesystem::path& sourcePath, const std::filesystem::path& destDir, EditorAssetDatabase& db);
    static std::vector<AssetId> ImportFbx(const std::filesystem::path& sourcePath, EditorAssetDatabase& db);

    /** Returns a .dasset.json path under dir with baseName that does not conflict with existing assets. */
    static std::filesystem::path ResolveDestPath(const std::filesystem::path& dir, const std::string& baseName, EditorAssetDatabase& db);
};

DELTA_ENGINE_NS_END
