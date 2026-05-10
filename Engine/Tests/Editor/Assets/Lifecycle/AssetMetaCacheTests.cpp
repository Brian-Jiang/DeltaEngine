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

class AssetMetaCacheTests : public EditorSerializationTest
{
};

TEST_F(AssetMetaCacheTests, ScanPopulatesMetaCacheFromDisk)
{
    const auto tempDir = MakeTempDir("DeltaMetaCacheScan");
    const AssetId assetId = AssetId::Generate();

    auto* asset = MakeAsset(assetId, "Hero");
    asset->SetDynamicMeta(nlohmann::json{
        {"desc", "hi"},
        {"tags", {"a", "b"}},
        {"extra", {{"k", "v"}}}
    });
    SaveAssetToFile(asset, tempDir.Path() / "Asset.dasset.json");

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir.Path());

    const auto& meta = db.GetAssetMeta(assetId);
    ASSERT_TRUE(meta.is_object());
    ASSERT_TRUE(meta.contains("dynamic"));
    EXPECT_EQ(meta["dynamic"]["desc"], "hi");
    ASSERT_TRUE(meta["dynamic"]["tags"].is_array());
    ASSERT_EQ(meta["dynamic"]["tags"].size(), 2u);
    EXPECT_EQ(meta["dynamic"]["tags"][0], "a");
    EXPECT_EQ(meta["dynamic"]["extra"]["k"], "v");
    ASSERT_TRUE(meta.contains("static"));
    EXPECT_TRUE(meta["static"].is_object());
}

TEST_F(AssetMetaCacheTests, ScanBackfillsMissingDynamicKeys)
{
    const auto tempDir = MakeTempDir("DeltaMetaCacheBackfill");
    const AssetId assetId = AssetId::Generate();

    auto* asset = MakeAsset(assetId, "Bare");
    const auto path = tempDir.Path() / "Bare.dasset.json";
    SaveAssetToFile(asset, path);

    nlohmann::json onDisk = ReadJson(path);
    onDisk["meta"]["dynamic"] = nlohmann::json::object();
    WriteJson(onDisk, path);

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir.Path());

    const auto& meta = db.GetAssetMeta(assetId);
    EXPECT_EQ(meta["dynamic"]["desc"], "");
    ASSERT_TRUE(meta["dynamic"]["tags"].is_array());
    EXPECT_EQ(meta["dynamic"]["tags"].size(), 0u);
}

TEST_F(AssetMetaCacheTests, ScanBackfillsCompletelyMissingMeta)
{
    const auto tempDir = MakeTempDir("DeltaMetaCacheNoMeta");
    const AssetId assetId = AssetId::Generate();

    auto* asset = MakeAsset(assetId, "Legacy");
    const auto path = tempDir.Path() / "Legacy.dasset.json";
    SaveAssetToFile(asset, path);

    nlohmann::json onDisk = ReadJson(path);
    onDisk.erase("meta");
    WriteJson(onDisk, path);

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir.Path());

    const auto& meta = db.GetAssetMeta(assetId);
    ASSERT_TRUE(meta.is_object());
    EXPECT_TRUE(meta["static"].is_object());
    EXPECT_TRUE(meta["static"].empty());
    EXPECT_EQ(meta["dynamic"]["desc"], "");
    ASSERT_TRUE(meta["dynamic"]["tags"].is_array());
    EXPECT_EQ(meta["dynamic"]["tags"].size(), 0u);
}

TEST_F(AssetMetaCacheTests, PartialLoadDoesNotInstantiateAsset)
{
    const auto tempDir = MakeTempDir("DeltaMetaCachePartial");
    const AssetId assetId = AssetId::Generate();

    auto* asset = MakeAsset(assetId, "NotLoaded");
    SaveAssetToFile(asset, tempDir.Path() / "NotLoaded.dasset.json");

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir.Path());

    EXPECT_EQ(db.GetState(assetId), EditorAssetDatabase::AssetState::HeaderOnly);
    EXPECT_EQ(db.GetLoadedAsset(assetId), nullptr);
    // Cache is still populated even though no instance exists.
    const auto& meta = db.GetAssetMeta(assetId);
    EXPECT_TRUE(meta.is_object());
    EXPECT_TRUE(meta["dynamic"].contains("desc"));
}

TEST_F(AssetMetaCacheTests, CreateAssetPopulatesCache)
{
    const auto tempDir = MakeTempDir("DeltaMetaCacheCreate");
    const AssetId assetId = AssetId::Generate();

    auto* asset = MakeAsset(assetId, "Created");
    asset->SetDynamicMeta(nlohmann::json{
        {"desc", "fresh"},
        {"tags", {"new"}}
    });

    EditorAssetDatabase db;
    db.CreateAsset(tempDir.Path() / "Created.dasset.json", asset);

    const auto& meta = db.GetAssetMeta(assetId);
    EXPECT_EQ(meta["dynamic"]["desc"], "fresh");
    ASSERT_TRUE(meta["dynamic"]["tags"].is_array());
    ASSERT_EQ(meta["dynamic"]["tags"].size(), 1u);
    EXPECT_EQ(meta["dynamic"]["tags"][0], "new");
}

TEST_F(AssetMetaCacheTests, SaveAssetRefreshesCache)
{
    const auto tempDir = MakeTempDir("DeltaMetaCacheSave");
    const AssetId assetId = AssetId::Generate();

    auto* asset = MakeAsset(assetId, "Mut");
    SaveAssetToFile(asset, tempDir.Path() / "Mut.dasset.json");

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir.Path());
    DPrimaryAsset* loaded = db.LoadAsset(assetId);
    ASSERT_NE(loaded, nullptr);

    loaded->SetDynamicMeta(nlohmann::json{
        {"desc", "after-save"},
        {"tags", {"x", "y"}}
    });
    loaded->MarkDirty();
    db.SaveAsset(assetId);

    const auto& meta = db.GetAssetMeta(assetId);
    EXPECT_EQ(meta["dynamic"]["desc"], "after-save");
    ASSERT_TRUE(meta["dynamic"]["tags"].is_array());
    EXPECT_EQ(meta["dynamic"]["tags"].size(), 2u);
}

TEST_F(AssetMetaCacheTests, LoadAssetRefreshesCacheFromLiveInstance)
{
    const auto tempDir = MakeTempDir("DeltaMetaCacheLoad");
    const AssetId assetId = AssetId::Generate();

    auto* asset = MakeAsset(assetId, "Partial");
    const auto path = tempDir.Path() / "Partial.dasset.json";
    SaveAssetToFile(asset, path);

    nlohmann::json onDisk = ReadJson(path);
    onDisk["meta"]["dynamic"] = nlohmann::json{ {"desc", "only-desc"} };
    WriteJson(onDisk, path);

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir.Path());
    DPrimaryAsset* loaded = db.LoadAsset(assetId);
    ASSERT_NE(loaded, nullptr);

    const auto& meta = db.GetAssetMeta(assetId);
    EXPECT_EQ(meta["dynamic"]["desc"], "only-desc");
    ASSERT_TRUE(meta["dynamic"]["tags"].is_array());
    EXPECT_EQ(meta["dynamic"]["tags"].size(), 0u);
}

TEST_F(AssetMetaCacheTests, DuplicateAssetCopiesMeta)
{
    const auto tempDir = MakeTempDir("DeltaMetaCacheDup");
    const AssetId assetId = AssetId::Generate();

    auto* asset = MakeAsset(assetId, "Source");
    asset->SetDynamicMeta(nlohmann::json{
        {"desc", "dup-source"},
        {"tags", {"alpha", "beta"}}
    });
    SaveAssetToFile(asset, tempDir.Path() / "Source.dasset.json");

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir.Path());
    ASSERT_NE(db.LoadAsset(assetId), nullptr);

    const AssetId dupId = db.DuplicateAsset(assetId);
    ASSERT_FALSE(dupId.IsNull());

    const auto& dupMeta = db.GetAssetMeta(dupId);
    EXPECT_EQ(dupMeta["dynamic"]["desc"], "dup-source");
    ASSERT_EQ(dupMeta["dynamic"]["tags"].size(), 2u);
    EXPECT_EQ(dupMeta["dynamic"]["tags"][0], "alpha");
    EXPECT_EQ(dupMeta["dynamic"]["tags"][1], "beta");
}

TEST_F(AssetMetaCacheTests, DeleteRemovesCache)
{
    const auto tempDir = MakeTempDir("DeltaMetaCacheDelete");
    const AssetId assetId = AssetId::Generate();

    auto* asset = MakeAsset(assetId, "Gone");
    SaveAssetToFile(asset, tempDir.Path() / "Gone.dasset.json");

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir.Path());
    EXPECT_TRUE(db.GetAssetMeta(assetId).contains("dynamic"));

    EXPECT_TRUE(db.DeleteAsset(assetId));

    const auto& meta = db.GetAssetMeta(assetId);
    ASSERT_TRUE(meta.is_object());
    EXPECT_EQ(meta["dynamic"]["desc"], "");
    EXPECT_EQ(meta["dynamic"]["tags"].size(), 0u);
}

TEST_F(AssetMetaCacheTests, RefreshAssetMetaCacheFromLiveAsset)
{
    const auto tempDir = MakeTempDir("DeltaMetaCacheManualRefresh");
    const AssetId assetId = AssetId::Generate();

    auto* asset = MakeAsset(assetId, "Refresh");
    SaveAssetToFile(asset, tempDir.Path() / "Refresh.dasset.json");

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir.Path());
    DPrimaryAsset* loaded = db.LoadAsset(assetId);
    ASSERT_NE(loaded, nullptr);

    loaded->SetDynamicMeta(nlohmann::json{
        {"desc", "manual"},
        {"tags", {"m"}}
    });
    db.RefreshAssetMetaCache(assetId);

    const auto& meta = db.GetAssetMeta(assetId);
    EXPECT_EQ(meta["dynamic"]["desc"], "manual");
    ASSERT_EQ(meta["dynamic"]["tags"].size(), 1u);
}
