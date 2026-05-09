#include "Shared/SerializationTestSupport.h"

#include "Runtime/Test/SerializationTestTypes.h"

#include <fstream>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

namespace
{
PA_TestAsset* MakeAsset(const AssetId& id, const char* name)
{
    auto* asset = CreateDObject<PA_TestAsset>();
    asset->GetHeader().m_persistentId = id;
    asset->GetHeader().m_className    = "PA_TestAsset";

    auto* objA = new DTestObjectA();
    objA->SetObjectId(ObjectId::Generate());
    objA->m_name = name;
    asset->AddObject(objA);
    return asset;
}

nlohmann::json ReadJson(const std::filesystem::path& path)
{
    std::ifstream in(path);
    nlohmann::json j;
    in >> j;
    return j;
}

void WriteJson(const nlohmann::json& j, const std::filesystem::path& path)
{
    std::ofstream out(path);
    out << j.dump(2);
}
}

class DynamicMetaRoundTripTests : public EditorSerializationTest
{
};

TEST_F(DynamicMetaRoundTripTests, RoundTripsNonTrivialDynamicMeta)
{
    const auto tempDir = MakeTempDir("DeltaDynamicMetaRoundTrip");
    const AssetId assetId = AssetId::Generate();

    auto* asset = MakeAsset(assetId, "Hero");
    asset->SetDynamicMeta(nlohmann::json{
        {"desc", "hello"},
        {"tags", {"a", "b", "c"}},
        {"extra", {{"author", "xj"}, {"rating", 5}}}
    });

    const auto filePath = tempDir.Path() / "Asset.dasset.json";
    SaveAssetToFile(asset, filePath);

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    DPrimaryAsset* loaded = database.LoadAsset(assetId);
    ASSERT_NE(loaded, nullptr);

    const auto& meta = loaded->GetDynamicMeta();
    EXPECT_EQ(meta["desc"], "hello");
    ASSERT_TRUE(meta["tags"].is_array());
    ASSERT_EQ(meta["tags"].size(), 3u);
    EXPECT_EQ(meta["tags"][0], "a");
    EXPECT_EQ(meta["tags"][1], "b");
    EXPECT_EQ(meta["tags"][2], "c");
    ASSERT_TRUE(meta["extra"].is_object());
    EXPECT_EQ(meta["extra"]["author"], "xj");
    EXPECT_EQ(meta["extra"]["rating"], 5);
}

TEST_F(DynamicMetaRoundTripTests, LegacyFileWithoutMetaLoadsWithDefaults)
{
    const auto tempDir = MakeTempDir("DeltaDynamicMetaLegacy");
    const AssetId assetId = AssetId::Generate();

    auto* asset = MakeAsset(assetId, "Legacy");
    const auto filePath = tempDir.Path() / "Legacy.dasset.json";
    SaveAssetToFile(asset, filePath);

    nlohmann::json onDisk = ReadJson(filePath);
    onDisk.erase("meta");
    WriteJson(onDisk, filePath);

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    DPrimaryAsset* loaded = database.LoadAsset(assetId);
    ASSERT_NE(loaded, nullptr);

    const auto& meta = loaded->GetDynamicMeta();
    ASSERT_TRUE(meta.is_object());
    EXPECT_EQ(meta["desc"], "");
    ASSERT_TRUE(meta["tags"].is_array());
    EXPECT_EQ(meta["tags"].size(), 0u);

    // Backfill is in-memory only — confirm the on-disk file still has no meta until re-saved.
    nlohmann::json afterLoad = ReadJson(filePath);
    EXPECT_FALSE(afterLoad.contains("meta"));
}

TEST_F(DynamicMetaRoundTripTests, WrongTypeDescAndTagsAreCoerced)
{
    const auto tempDir = MakeTempDir("DeltaDynamicMetaCoerce");
    const AssetId assetId = AssetId::Generate();

    auto* asset = MakeAsset(assetId, "Coerce");
    const auto filePath = tempDir.Path() / "Coerce.dasset.json";
    SaveAssetToFile(asset, filePath);

    nlohmann::json onDisk = ReadJson(filePath);
    onDisk["meta"]["dynamic"] = nlohmann::json{
        {"desc", 42},
        {"tags", "notArray"},
        {"ok", "keep"}
    };
    WriteJson(onDisk, filePath);

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    DPrimaryAsset* loaded = database.LoadAsset(assetId);
    ASSERT_NE(loaded, nullptr);

    const auto& meta = loaded->GetDynamicMeta();
    EXPECT_EQ(meta["desc"], "");
    ASSERT_TRUE(meta["tags"].is_array());
    EXPECT_EQ(meta["tags"].size(), 0u);
    EXPECT_EQ(meta["ok"], "keep");
}

TEST_F(DynamicMetaRoundTripTests, TagsArrayWithNonStringEntriesGetsCleaned)
{
    const auto tempDir = MakeTempDir("DeltaDynamicMetaTagsCleanup");
    const AssetId assetId = AssetId::Generate();

    auto* asset = MakeAsset(assetId, "Tags");
    const auto filePath = tempDir.Path() / "Tags.dasset.json";
    SaveAssetToFile(asset, filePath);

    nlohmann::json onDisk = ReadJson(filePath);
    onDisk["meta"]["dynamic"] = nlohmann::json{
        {"desc", "ok"},
        {"tags", {"good", 7, "alsoGood", nullptr}}
    };
    WriteJson(onDisk, filePath);

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    DPrimaryAsset* loaded = database.LoadAsset(assetId);
    ASSERT_NE(loaded, nullptr);

    const auto& meta = loaded->GetDynamicMeta();
    EXPECT_EQ(meta["desc"], "ok");
    ASSERT_TRUE(meta["tags"].is_array());
    ASSERT_EQ(meta["tags"].size(), 2u);
    EXPECT_EQ(meta["tags"][0], "good");
    EXPECT_EQ(meta["tags"][1], "alsoGood");
}

TEST_F(DynamicMetaRoundTripTests, DuplicateAssetPreservesDynamicMeta)
{
    const auto tempDir = MakeTempDir("DeltaDynamicMetaDuplicate");
    const AssetId assetId = AssetId::Generate();

    auto* asset = MakeAsset(assetId, "Source");
    nlohmann::json richMeta{
        {"desc", "source description"},
        {"tags", {"alpha", "beta"}},
        {"nested", {{"k", "v"}, {"n", 17}}}
    };
    asset->SetDynamicMeta(richMeta);

    const auto filePath = tempDir.Path() / "Source.dasset.json";
    SaveAssetToFile(asset, filePath);

    EditorAssetDatabase database;
    database.ScanAssetsFolder(tempDir.Path());
    ASSERT_NE(database.LoadAsset(assetId), nullptr);

    const AssetId duplicatedId = database.DuplicateAsset(assetId);
    ASSERT_FALSE(duplicatedId.IsNull());

    DPrimaryAsset* duplicated = database.LoadAsset(duplicatedId);
    ASSERT_NE(duplicated, nullptr);

    const auto& dupMeta = duplicated->GetDynamicMeta();
    EXPECT_EQ(dupMeta["desc"], richMeta["desc"]);
    EXPECT_EQ(dupMeta["tags"], richMeta["tags"]);
    EXPECT_EQ(dupMeta["nested"], richMeta["nested"]);
}

TEST_F(DynamicMetaRoundTripTests, SaveAssetEmitsStaticEmptyAndDynamicPresent)
{
    const auto tempDir = MakeTempDir("DeltaDynamicMetaShape");
    const AssetId assetId = AssetId::Generate();

    auto* asset = MakeAsset(assetId, "Shape");
    asset->SetDynamicMeta(nlohmann::json{
        {"desc", "shape"},
        {"tags", {"x"}}
    });

    const auto filePath = tempDir.Path() / "Shape.dasset.json";
    SaveAssetToFile(asset, filePath);

    nlohmann::json onDisk = ReadJson(filePath);
    ASSERT_TRUE(onDisk.contains("meta"));
    ASSERT_TRUE(onDisk["meta"].is_object());
    ASSERT_TRUE(onDisk["meta"].contains("static"));
    ASSERT_TRUE(onDisk["meta"]["static"].is_object());
    EXPECT_TRUE(onDisk["meta"]["static"].empty());
    ASSERT_TRUE(onDisk["meta"].contains("dynamic"));
    EXPECT_EQ(onDisk["meta"]["dynamic"]["desc"], "shape");
    ASSERT_TRUE(onDisk["meta"]["dynamic"]["tags"].is_array());
    ASSERT_EQ(onDisk["meta"]["dynamic"]["tags"].size(), 1u);
    EXPECT_EQ(onDisk["meta"]["dynamic"]["tags"][0], "x");
}
