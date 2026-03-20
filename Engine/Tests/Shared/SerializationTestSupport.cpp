#include "Shared/SerializationTestSupport.h"

#include "Runtime/Assets/AssetDatabaseLocator.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Serialization/JsonAssetArchive.h"

#include <fstream>
#include <system_error>

namespace DeltaEngine::Tests
{
ScopedTempDir::ScopedTempDir(std::string_view prefix)
{
    m_path = std::filesystem::temp_directory_path() /
        (std::string(prefix) + "_" + UUID::Generate().ToString());
    std::filesystem::create_directories(m_path);
}

ScopedTempDir::~ScopedTempDir()
{
    Cleanup();
}

ScopedTempDir::ScopedTempDir(ScopedTempDir&& other) noexcept
    : m_path(std::move(other.m_path))
{
    other.m_path.clear();
}

ScopedTempDir& ScopedTempDir::operator=(ScopedTempDir&& other) noexcept
{
    if (this == &other)
        return *this;

    Cleanup();
    m_path = std::move(other.m_path);
    other.m_path.clear();
    return *this;
}

const std::filesystem::path& ScopedTempDir::Path() const
{
    return m_path;
}

void ScopedTempDir::Cleanup()
{
    if (m_path.empty())
        return;

    std::error_code ec;
    std::filesystem::remove_all(m_path, ec);
    m_path.clear();
}

ScopedTempDir EditorSerializationTest::MakeTempDir(std::string_view prefix) const
{
    return ScopedTempDir(prefix);
}

EditorAssetDatabase& EditorSerializationTest::GetRegisteredEditorAssetDatabase()
{
    static EditorAssetDatabase db;
    static bool registered = false;

    if (!registered)
    {
        AssetDatabaseLocator::Register(&db);
        registered = true;
    }

    return db;
}

void EditorSerializationTest::SaveAssetToFile(
    DPrimaryAsset* asset,
    const std::filesystem::path& filePath)
{
    std::filesystem::create_directories(filePath.parent_path());

    JsonAssetArchive headerArchive;
    asset->SerializeHeader(headerArchive);

    JsonAssetArchive bodyArchive;
    asset->SerializeBody(bodyArchive);

    nlohmann::json output;
    output["header"] = headerArchive.GetRoot();
    for (auto& [key, value] : bodyArchive.GetRoot().items())
        output[key] = value;

    std::ofstream outputStream(filePath);
    outputStream << output.dump(2);
}

void EditorSerializationTest::SaveAssetWithBulkData(
    DPrimaryAsset* asset,
    const std::filesystem::path& filePath)
{
    std::filesystem::create_directories(filePath.parent_path());

    const auto assetDir = filePath.parent_path();
    const auto assetStem = filePath.stem().stem().string();

    JsonAssetArchive bulkArchive(assetDir, assetStem);
    asset->SerializeBulkData(bulkArchive);

    JsonAssetArchive headerArchive;
    asset->SerializeHeader(headerArchive);

    JsonAssetArchive bodyArchive;
    asset->SerializeBody(bodyArchive);

    nlohmann::json output;
    output["header"] = headerArchive.GetRoot();

    const auto& bulkRoot = bulkArchive.GetRoot();
    if (bulkRoot.contains("header") && bulkRoot["header"].contains("bulkDataMap"))
        output["header"]["bulkDataMap"] = bulkRoot["header"]["bulkDataMap"];

    for (auto& [key, value] : bodyArchive.GetRoot().items())
        output[key] = value;

    std::ofstream outputStream(filePath);
    outputStream << output.dump(2);
}

void EditorSerializationTest::SaveJsonToFile(
    const nlohmann::json& json,
    const std::filesystem::path& filePath)
{
    std::filesystem::create_directories(filePath.parent_path());

    std::ofstream outputStream(filePath);
    outputStream << json.dump(2);
}

DProperty* EditorSerializationTest::FindProperty(DObject* object, const std::string& propertyName)
{
    if (!object)
        return nullptr;

    DClass* dclass = object->GetClass();
    return dclass ? dclass->FindPropertyByName(propertyName) : nullptr;
}
}
