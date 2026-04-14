#pragma once

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Core/UUID.h"
#include "Runtime/Reflection/DProperty.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <string>
#include <string_view>

namespace DeltaEngine::Tests
{
class ScopedTempDir
{
public:
    explicit ScopedTempDir(std::string_view prefix);
    ~ScopedTempDir();

    ScopedTempDir(const ScopedTempDir&) = delete;
    ScopedTempDir& operator=(const ScopedTempDir&) = delete;

    ScopedTempDir(ScopedTempDir&& other) noexcept;
    ScopedTempDir& operator=(ScopedTempDir&& other) noexcept;

    const std::filesystem::path& Path() const;

private:
    void Cleanup();

    std::filesystem::path m_path;
};

class EditorSerializationTest : public ::testing::Test
{
protected:
    ScopedTempDir MakeTempDir(std::string_view prefix) const;

    // Process-wide singleton for tests; does not call AssetDatabaseLocator::Register.
    static EditorAssetDatabase& GetSharedEditorAssetDatabase();
    static void SaveAssetToFile(DPrimaryAsset* asset, const std::filesystem::path& filePath);
    static void SaveAssetWithBulkData(DPrimaryAsset* asset, const std::filesystem::path& filePath);
    static void SaveJsonToFile(const nlohmann::json& json, const std::filesystem::path& filePath);
    static DProperty* FindProperty(DObject* object, const std::string& propertyName);

    template <typename TObject>
    static TObject* FindObjectAs(DPrimaryAsset* asset, const ObjectId& objectId)
    {
        return asset ? dynamic_cast<TObject*>(asset->FindObject(objectId)) : nullptr;
    }
};
}
