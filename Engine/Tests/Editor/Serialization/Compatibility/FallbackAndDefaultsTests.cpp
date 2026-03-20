#include "Shared/SerializationTestSupport.h"

#include "Runtime/Reflection/DClass.h"

#include "Runtime/Test/SerializationTestTypes.h"

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class FallbackAndDefaultsTests : public EditorSerializationTest
{
};

TEST_F(FallbackAndDefaultsTests, UnknownAssetClassFallsBackToBaseAsset)
{
    const auto tempDir = MakeTempDir("DeltaUnknownAssetClassTest");

    const AssetId assetId = AssetId::Generate();
    const nlohmann::json file = {
        {"header", {
            {"magic", "DLTA"},
            {"version", 1},
            {"className", "PA_MissingAsset"},
            {"assetId", assetId.ToString()}
        }},
        {"objects", nlohmann::json::array()}
    };

    SaveJsonToFile(file, tempDir.Path() / "UnknownAssetClass.dasset.json");

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    DPrimaryAsset* loaded = database.LoadAsset(assetId);

    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetClass()->GetName(), "DPrimaryAsset");
    EXPECT_EQ(loaded->GetHeader().m_className, "PA_MissingAsset");
}

TEST_F(FallbackAndDefaultsTests, ExtraDataIsIgnoredDuringLoad)
{
    const auto tempDir = MakeTempDir("DeltaExtraDataTest");

    const AssetId assetId = AssetId::Generate();
    const std::string objectId = ObjectId::Generate().ToString();

    const nlohmann::json file = {
        {"header", {
            {"magic", "DLTA"},
            {"version", 1},
            {"className", "PA_TestAsset"},
            {"assetId", assetId.ToString()}
        }},
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class", "DTestObjectA"},
                {"_objectId", objectId},
                {"m_health", 42.0f},
                {"m_level", 7},
                {"m_mana", 999.0f},
                {"m_buffs", "arcane"},
                {"m_name", "Wizard"}
            }
        })}
    };

    SaveJsonToFile(file, tempDir.Path() / "Extra.dasset.json");

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    DPrimaryAsset* loaded = database.LoadAsset(assetId);

    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetObjects().size(), 1u);

    auto* object = FindObjectAs<DTestObjectA>(loaded, ObjectId::FromString(objectId));
    ASSERT_NE(object, nullptr);
    EXPECT_FLOAT_EQ(object->m_health, 42.0f);
    EXPECT_EQ(object->m_level, 7);
    EXPECT_EQ(object->m_name, "Wizard");
}

TEST_F(FallbackAndDefaultsTests, MissingDataUsesCppDefaults)
{
    const auto tempDir = MakeTempDir("DeltaMissingDataTest");

    const AssetId assetId = AssetId::Generate();
    const std::string objectId = ObjectId::Generate().ToString();

    const nlohmann::json file = {
        {"header", {
            {"magic", "DLTA"},
            {"version", 1},
            {"className", "PA_TestAsset"},
            {"assetId", assetId.ToString()}
        }},
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class", "DTestObjectA"},
                {"_objectId", objectId},
                {"m_health", 25.0f}
            }
        })}
    };

    SaveJsonToFile(file, tempDir.Path() / "Missing.dasset.json");

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    DPrimaryAsset* loaded = database.LoadAsset(assetId);

    ASSERT_NE(loaded, nullptr);
    auto* object = FindObjectAs<DTestObjectA>(loaded, ObjectId::FromString(objectId));
    ASSERT_NE(object, nullptr);
    EXPECT_FLOAT_EQ(object->m_health, 25.0f);
    EXPECT_EQ(object->m_level, 1);
    EXPECT_FALSE(object->m_isActive);
    EXPECT_NEAR(object->m_stamina, 50.0, 1e-10);
    EXPECT_EQ(object->m_name, "DefaultName");
}

TEST_F(FallbackAndDefaultsTests, UnknownClassEntriesAreSkipped)
{
    const auto tempDir = MakeTempDir("DeltaUnknownClassTest");

    const AssetId assetId = AssetId::Generate();
    const std::string validObjectId = ObjectId::Generate().ToString();

    const nlohmann::json file = {
        {"header", {
            {"magic", "DLTA"},
            {"version", 1},
            {"className", "PA_TestAsset"},
            {"assetId", assetId.ToString()}
        }},
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class", "DDeletedClass"},
                {"_objectId", ObjectId::Generate().ToString()},
                {"m_foo", 123}
            },
            nlohmann::json{
                {"_class", "DTestObjectA"},
                {"_objectId", validObjectId},
                {"m_health", 60.0f}
            }
        })}
    };

    SaveJsonToFile(file, tempDir.Path() / "Unknown.dasset.json");

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    DPrimaryAsset* loaded = database.LoadAsset(assetId);

    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->GetObjects().size(), 1u);

    auto* object = FindObjectAs<DTestObjectA>(loaded, ObjectId::FromString(validObjectId));
    ASSERT_NE(object, nullptr);
    EXPECT_FLOAT_EQ(object->m_health, 60.0f);
}
