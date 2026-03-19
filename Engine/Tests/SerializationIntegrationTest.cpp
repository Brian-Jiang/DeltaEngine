#include "Runtime/Test/SerializationTestTypes.h"
#include "Runtime/Assets/AssetDatabaseLocator.h"
#include "Editor/Assets/EditorAssetDatabase.h"
#include "Runtime/Serialization/JsonAssetArchive.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"

#include <nlohmann/json.hpp>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <iostream>

using namespace DeltaEngine;
using DeltaEngine::UUID;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::filesystem::path MakeTempDir(const char* name)
{
    std::cout << "Temp directory is " << std::filesystem::temp_directory_path() << '\n';
    auto dir = std::filesystem::temp_directory_path() / name;
    std::filesystem::create_directories(dir);
    return dir;
}

static void SaveAssetToFile(
    DPrimaryAsset* asset,
    const std::filesystem::path& filePath)
{
    JsonAssetArchive headerAr;
    asset->SerializeHeader(headerAr);

    JsonAssetArchive bodyAr;
    asset->SerializeBody(bodyAr);

    nlohmann::json output;
    output["header"] = headerAr.GetRoot();
    for (auto& [key, val] : bodyAr.GetRoot().items())
        output[key] = val;

    std::ofstream out(filePath);
    out << output.dump(2);
}

static void SaveAssetWithBulkData(
    DPrimaryAsset* asset,
    const std::filesystem::path& filePath)
{
    const auto assetDir  = filePath.parent_path();
    const auto assetStem = filePath.stem().stem().string();

    JsonAssetArchive bulkAr(assetDir, assetStem);
    asset->SerializeBulkData(bulkAr);

    JsonAssetArchive headerAr;
    asset->SerializeHeader(headerAr);

    JsonAssetArchive bodyAr;
    asset->SerializeBody(bodyAr);

    nlohmann::json output;
    output["header"] = headerAr.GetRoot();
    const auto& bulkRoot = bulkAr.GetRoot();
    if (bulkRoot.contains("header") && bulkRoot["header"].contains("bulkDataMap"))
        output["header"]["bulkDataMap"] = bulkRoot["header"]["bulkDataMap"];
    for (auto& [key, val] : bodyAr.GetRoot().items())
        output[key] = val;

    std::ofstream out(filePath);
    out << output.dump(2);
}

static EditorAssetDatabase& GetRegisteredEditorAssetDatabase()
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

// ---------------------------------------------------------------------------
// Test 1: Single-asset round-trip with mixed properties
// ---------------------------------------------------------------------------

static void TestAssetDatabaseLocatorWithEditorAssetDatabase()
{
    auto tempDir = MakeTempDir("DeltaLocatorEditorDbTest");

    auto* asset = CreateDObject<PA_TestAsset>();
    AssetId assetId = UUID::Generate();
    asset->GetHeader().m_persistentId = assetId;
    asset->GetHeader().m_className    = "PA_TestAsset";

    auto* rawA = new DTestObjectA();
    ObjectId objAId = UUID::Generate();
    rawA->SetObjectId(objAId);
    rawA->m_name = "LocatorHero";
    asset->AddObject(rawA);

    auto filePath = tempDir / "LocatorAsset.dasset.json";
    SaveAssetToFile(asset, filePath);

    EditorAssetDatabase& db = GetRegisteredEditorAssetDatabase();
    db.ScanAssetsFolder(tempDir);

    IAssetDatabase& locatorDb = AssetDatabaseLocator::Get();
    assert(&locatorDb == &db);
    assert(!locatorDb.IsLoaded(assetId));

    auto* loaded = locatorDb.LoadAsset(assetId);
    assert(loaded != nullptr);
    assert(locatorDb.IsLoaded(assetId));

    auto* loadedA = dynamic_cast<DTestObjectA*>(locatorDb.FindObject(assetId, objAId));
    assert(loadedA != nullptr);
    assert(loadedA->m_name == "LocatorHero");

    std::filesystem::remove_all(tempDir);
    std::cout << "[PASS] TestAssetDatabaseLocatorWithEditorAssetDatabase\n";
}

static void TestSingleAssetRoundTrip()
{
    auto tempDir = MakeTempDir("DeltaRoundTripTest");

    auto* asset = CreateDObject<PA_TestAsset>();
    AssetId assetId = UUID::Generate();
    asset->GetHeader().m_persistentId = assetId;
    asset->GetHeader().m_className    = "PA_TestAsset";

    auto* rawA = new DTestObjectA();
    ObjectId objAId = UUID::Generate();
    rawA->SetObjectId(objAId);
    rawA->m_health   = 75.5f;
    rawA->m_level    = 10;
    rawA->m_isActive = true;
    rawA->m_stamina  = 88.8;
    rawA->m_name     = "TestHero";
    rawA->m_position = { 1.0f, 2.0f, 3.0f };
    rawA->m_rotation = { 0.0f, 0.707f, 0.0f, 0.707f };
    rawA->m_weights  = { 1.0f, 2.0f, 3.0f };
    asset->AddObject(rawA);

    auto* rawB = new DTestObjectB();
    ObjectId objBId = UUID::Generate();
    rawB->SetObjectId(objBId);
    rawB->m_label     = "Companion";
    rawB->m_weight    = 3.14f;
    rawB->m_targetRef = rawA;
    asset->AddObject(rawB);

    auto filePath = tempDir / "TestAsset.dasset.json";
    SaveAssetToFile(asset, filePath);

    assert(std::filesystem::exists(filePath));

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir);
    assert(db.GetState(assetId) != EditorAssetDatabase::AssetState::Unregistered);

    auto loaded = db.LoadAsset(assetId);
    assert(loaded != nullptr);
    assert(loaded->GetObjects().size() == 2);

    auto* loadedA = dynamic_cast<DTestObjectA*>(loaded->FindObject(objAId));
    assert(loadedA != nullptr);
    assert(loadedA->m_health == 75.5f);
    assert(loadedA->m_level == 10);
    assert(loadedA->m_isActive == true);
    assert(std::abs(loadedA->m_stamina - 88.8) < 1e-10);
    assert(loadedA->m_name == "TestHero");
    assert(loadedA->m_position.x == 1.0f);
    assert(loadedA->m_position.y == 2.0f);
    assert(loadedA->m_position.z == 3.0f);
    assert(std::abs(loadedA->m_rotation.x - 0.0f)   < 1e-5f);
    assert(std::abs(loadedA->m_rotation.y - 0.707f)  < 1e-3f);
    assert(std::abs(loadedA->m_rotation.z - 0.0f)   < 1e-5f);
    assert(std::abs(loadedA->m_rotation.w - 0.707f)  < 1e-3f);
    assert(loadedA->m_weights.size() == 3);
    assert(loadedA->m_weights[0] == 1.0f);
    assert(loadedA->m_weights[1] == 2.0f);
    assert(loadedA->m_weights[2] == 3.0f);

    auto* loadedB = dynamic_cast<DTestObjectB*>(loaded->FindObject(objBId));
    assert(loadedB != nullptr);
    assert(loadedB->m_label == "Companion");
    assert(std::abs(loadedB->m_weight - 3.14f) < 1e-5f);
    assert(loadedB->m_targetRef == loadedA);

    std::filesystem::remove_all(tempDir);
    std::cout << "[PASS] TestSingleAssetRoundTrip\n";
}

// ---------------------------------------------------------------------------
// Test 2: Extra data in file is ignored, no crash
// ---------------------------------------------------------------------------

static void TestExtraDataIgnored()
{
    auto tempDir = MakeTempDir("DeltaExtraDataTest");

    AssetId testId = UUID::Generate();
    std::string objIdStr = UUID::Generate().ToString();

    nlohmann::json file = {
        {"header", {
            {"magic",     "DLTA"},
            {"version",   1},
            {"className", "PA_TestAsset"},
            {"assetId",   testId.ToString()}
        }},
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class",    "DTestObjectA"},
                {"_objectId", objIdStr},
                {"m_health",  42.0f},
                {"m_level",   7},
                {"m_mana",    999.0f},
                {"m_buffs",   "arcane"},
                {"m_name",    "Wizard"}
            }
        })}
    };

    {
        std::ofstream out(tempDir / "Extra.dasset.json");
        out << file.dump(2);
    }

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir);
    auto loaded = db.LoadAsset(testId);

    assert(loaded != nullptr);
    assert(loaded->GetObjects().size() == 1);

    ObjectId objId = UUID::FromString(objIdStr);
    auto* obj = dynamic_cast<DTestObjectA*>(loaded->FindObject(objId));
    assert(obj != nullptr);
    assert(obj->m_health == 42.0f);
    assert(obj->m_level  == 7);
    assert(obj->m_name   == "Wizard");

    std::filesystem::remove_all(tempDir);
    std::cout << "[PASS] TestExtraDataIgnored\n";
}

// ---------------------------------------------------------------------------
// Test 3: Missing data uses C++ defaults
// ---------------------------------------------------------------------------

static void TestMissingDataUsesDefaults()
{
    auto tempDir = MakeTempDir("DeltaMissingDataTest");

    AssetId testId   = UUID::Generate();
    std::string objIdStr = UUID::Generate().ToString();

    nlohmann::json file = {
        {"header", {
            {"magic",     "DLTA"},
            {"version",   1},
            {"className", "PA_TestAsset"},
            {"assetId",   testId.ToString()}
        }},
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class",    "DTestObjectA"},
                {"_objectId", objIdStr},
                {"m_health",  25.0f}
            }
        })}
    };

    {
        std::ofstream out(tempDir / "Missing.dasset.json");
        out << file.dump(2);
    }

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir);
    auto loaded = db.LoadAsset(testId);
    assert(loaded != nullptr);

    ObjectId objId = UUID::FromString(objIdStr);
    auto* obj = dynamic_cast<DTestObjectA*>(loaded->FindObject(objId));
    assert(obj != nullptr);
    assert(obj->m_health   == 25.0f);
    assert(obj->m_level    == 1);
    assert(obj->m_isActive == false);
    assert(std::abs(obj->m_stamina - 50.0) < 1e-10);
    assert(obj->m_name     == "DefaultName");

    std::filesystem::remove_all(tempDir);
    std::cout << "[PASS] TestMissingDataUsesDefaults\n";
}

// ---------------------------------------------------------------------------
// Test 4: Unknown class in file is skipped gracefully
// ---------------------------------------------------------------------------

static void TestUnknownClassSkipped()
{
    auto tempDir = MakeTempDir("DeltaUnknownClassTest");

    AssetId testId = UUID::Generate();
    std::string validObjIdStr = UUID::Generate().ToString();

    nlohmann::json file = {
        {"header", {
            {"magic",     "DLTA"},
            {"version",   1},
            {"className", "PA_TestAsset"},
            {"assetId",   testId.ToString()}
        }},
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class",    "DDeletedClass"},
                {"_objectId", UUID::Generate().ToString()},
                {"m_foo",     123}
            },
            nlohmann::json{
                {"_class",    "DTestObjectA"},
                {"_objectId", validObjIdStr},
                {"m_health",  60.0f}
            }
        })}
    };

    {
        std::ofstream out(tempDir / "Unknown.dasset.json");
        out << file.dump(2);
    }

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir);
    auto loaded = db.LoadAsset(testId);

    assert(loaded != nullptr);
    assert(loaded->GetObjects().size() == 1);

    ObjectId validId = UUID::FromString(validObjIdStr);
    auto* obj = dynamic_cast<DTestObjectA*>(loaded->FindObject(validId));
    assert(obj != nullptr);
    assert(obj->m_health == 60.0f);

    std::filesystem::remove_all(tempDir);
    std::cout << "[PASS] TestUnknownClassSkipped\n";
}

// ---------------------------------------------------------------------------
// Test 5: Cross-asset pointer resolution
// ---------------------------------------------------------------------------

static void TestCrossAssetPointerResolution()
{
    auto tempDir = MakeTempDir("DeltaCrossAssetTest");

    AssetId idA     = UUID::Generate();
    ObjectId objIdA = UUID::Generate();

    AssetId idB     = UUID::Generate();
    ObjectId objIdB = UUID::Generate();

    nlohmann::json fileA = {
        {"header", {
            {"magic",     "DLTA"},
            {"version",   1},
            {"className", "PA_TestAsset"},
            {"assetId",   idA.ToString()}
        }},
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class",    "DTestObjectA"},
                {"_objectId", objIdA.ToString()},
                {"m_health",  80.0f},
                {"m_name",    "TargetObj"}
            }
        })}
    };

    nlohmann::json fileB = {
        {"header", {
            {"magic",     "DLTA"},
            {"version",   1},
            {"className", "PA_TestAsset"},
            {"assetId",   idB.ToString()}
        }},
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class",    "DTestObjectB"},
                {"_objectId", objIdB.ToString()},
                {"m_label",   "OwnerObj"},
                {"m_weight",  2.0f},
                {"m_targetRef", nlohmann::json{
                    {"assetId",  idA.ToString()},
                    {"objectId", objIdA.ToString()}
                }}
            }
        })}
    };

    {
        std::ofstream(tempDir / "AssetA.dasset.json") << fileA.dump(2);
        std::ofstream(tempDir / "AssetB.dasset.json") << fileB.dump(2);
    }

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir);

    auto loadedB = db.LoadAsset(idB);
    assert(loadedB != nullptr);
    assert(db.IsLoaded(idA));
    assert(db.IsLoaded(idB));

    auto* objB = dynamic_cast<DTestObjectB*>(loadedB->FindObject(objIdB));
    assert(objB != nullptr);
    assert(objB->m_label == "OwnerObj");
    assert(objB->m_targetRef != nullptr);
    assert(objB->m_targetRef->GetObjectId() == objIdA);

    auto* objA = dynamic_cast<DTestObjectA*>(objB->m_targetRef);
    assert(objA != nullptr);
    assert(objA->m_name == "TargetObj");

    std::filesystem::remove_all(tempDir);
    std::cout << "[PASS] TestCrossAssetPointerResolution\n";
}

// ---------------------------------------------------------------------------
// Test 6: Cyclic dependency loads without infinite recursion
// ---------------------------------------------------------------------------

static void TestCyclicDependency()
{
    auto tempDir = MakeTempDir("DeltaCyclicTest");

    AssetId idA     = UUID::Generate();
    ObjectId objIdA = UUID::Generate();
    AssetId idB     = UUID::Generate();
    ObjectId objIdB = UUID::Generate();

    nlohmann::json fileA = {
        {"header", {
            {"magic",     "DLTA"},
            {"version",   1},
            {"className", "PA_TestAsset"},
            {"assetId",   idA.ToString()}
        }},
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class",    "DTestObjectB"},
                {"_objectId", objIdA.ToString()},
                {"m_label",   "InAssetA"},
                {"m_targetRef", nlohmann::json{
                    {"assetId",  idB.ToString()},
                    {"objectId", objIdB.ToString()}
                }}
            }
        })}
    };

    nlohmann::json fileB = {
        {"header", {
            {"magic",     "DLTA"},
            {"version",   1},
            {"className", "PA_TestAsset"},
            {"assetId",   idB.ToString()}
        }},
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class",    "DTestObjectB"},
                {"_objectId", objIdB.ToString()},
                {"m_label",   "InAssetB"},
                {"m_targetRef", nlohmann::json{
                    {"assetId",  idA.ToString()},
                    {"objectId", objIdA.ToString()}
                }}
            }
        })}
    };

    {
        std::ofstream(tempDir / "CycleA.dasset.json") << fileA.dump(2);
        std::ofstream(tempDir / "CycleB.dasset.json") << fileB.dump(2);
    }

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir);

    auto loadedA = db.LoadAsset(idA);
    assert(loadedA != nullptr);
    assert(db.IsLoaded(idA));
    assert(db.IsLoaded(idB));

    auto* oA = dynamic_cast<DTestObjectB*>(loadedA->FindObject(objIdA));
    assert(oA != nullptr);
    assert(oA->m_targetRef != nullptr);
    assert(oA->m_targetRef->GetObjectId() == objIdB);

    auto loadedB = db.LoadAsset(idB);
    auto* oB = dynamic_cast<DTestObjectB*>(loadedB->FindObject(objIdB));
    assert(oB != nullptr);
    assert(oB->m_targetRef != nullptr);
    assert(oB->m_targetRef->GetObjectId() == objIdA);

    std::filesystem::remove_all(tempDir);
    std::cout << "[PASS] TestCyclicDependency\n";
}

// ---------------------------------------------------------------------------
// Test 7: Dirty flag propagation via DProperty::SetValue
// ---------------------------------------------------------------------------

static void TestDirtyFlagPropagation()
{
    auto* asset = CreateDObject<PA_TestAsset>();
    asset->GetHeader().m_persistentId = UUID::Generate();
    asset->GetHeader().m_className    = "PA_TestAsset";

    auto* rawObj = new DTestObjectA();
    rawObj->SetObjectId(UUID::Generate());
    asset->AddObject(rawObj);

    asset->ClearDirty();
    assert(!asset->IsDirty());

    DClass* dclass = rawObj->GetClass();
    assert(dclass != nullptr);

    DFloatProperty* healthProp = nullptr;
    for (DProperty* p = dclass->GetOwnProperties(); p; p = p->GetNext())
    {
        if (p->GetName() == "m_health")
        {
            healthProp = dynamic_cast<DFloatProperty*>(p);
            break;
        }
    }
    assert(healthProp != nullptr);

    float newVal = 50.0f;
    healthProp->SetValue(rawObj, &newVal);

    assert(rawObj->m_health == 50.0f);
    assert(asset->IsDirty());

    std::cout << "[PASS] TestDirtyFlagPropagation\n";
}

// ---------------------------------------------------------------------------
// Test 8: Broken reference resolves to nullptr
// ---------------------------------------------------------------------------

static void TestBrokenReferenceIsNullptr()
{
    auto tempDir = MakeTempDir("DeltaBrokenRefTest");

    AssetId idA     = UUID::Generate();
    ObjectId objIdA = UUID::Generate();

    nlohmann::json file = {
        {"header", {
            {"magic",     "DLTA"},
            {"version",   1},
            {"className", "PA_TestAsset"},
            {"assetId",   idA.ToString()}
        }},
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class",    "DTestObjectB"},
                {"_objectId", objIdA.ToString()},
                {"m_label",   "HasBrokenRef"},
                {"m_targetRef", nlohmann::json{
                    {"assetId",  UUID::Generate().ToString()},
                    {"objectId", UUID::Generate().ToString()}
                }}
            }
        })}
    };

    {
        std::ofstream(tempDir / "BrokenRef.dasset.json") << file.dump(2);
    }

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir);
    auto loaded = db.LoadAsset(idA);

    assert(loaded != nullptr);
    auto* obj = dynamic_cast<DTestObjectB*>(loaded->FindObject(objIdA));
    assert(obj != nullptr);
    assert(obj->m_targetRef == nullptr);

    std::filesystem::remove_all(tempDir);
    std::cout << "[PASS] TestBrokenReferenceIsNullptr\n";
}

// ---------------------------------------------------------------------------
// Test 9: Bulk data round-trip
// ---------------------------------------------------------------------------

static void TestBulkDataRoundTrip()
{
    auto tempDir = MakeTempDir("DeltaBulkTest");

    auto* asset = CreateDObject<PA_TestMesh>();
    AssetId assetId = UUID::Generate();
    asset->GetHeader().m_persistentId = assetId;
    asset->GetHeader().m_className    = "PA_TestMesh";

    auto* rawMesh = new DTestMeshData();
    ObjectId meshId = UUID::Generate();
    rawMesh->SetObjectId(meshId);
    rawMesh->m_vertexCount = 100;
    rawMesh->m_indexCount  = 300;

    std::vector<uint8_t> verts(400);
    for (int i = 0; i < 400; ++i) verts[i] = static_cast<uint8_t>(i % 256);
    rawMesh->m_vertexBuffer.Set(verts.data(), static_cast<uint64_t>(verts.size()));

    std::vector<uint8_t> indices(600);
    for (int i = 0; i < 600; ++i) indices[i] = static_cast<uint8_t>((i * 7) % 256);
    rawMesh->m_indexBuffer.Set(indices.data(), static_cast<uint64_t>(indices.size()));

    asset->AddObject(rawMesh);

    auto filePath = tempDir / "TestMesh.dasset.json";
    SaveAssetWithBulkData(asset, filePath);

    assert(std::filesystem::exists(tempDir / "TestMesh_Bulk0.bin"));
    assert(std::filesystem::exists(tempDir / "TestMesh_Bulk1.bin"));
    assert(std::filesystem::file_size(tempDir / "TestMesh_Bulk0.bin") == 400);
    assert(std::filesystem::file_size(tempDir / "TestMesh_Bulk1.bin") == 600);

    {
        std::ifstream in(filePath);
        auto j = nlohmann::json::parse(in);
        assert(j["objects"][0]["m_vertexBuffer"]["_bulk"] == 0);
        assert(j["objects"][0]["m_vertexBuffer"]["size"]  == 400);
        assert(j["objects"][0]["m_indexBuffer"]["_bulk"]  == 1);
        assert(j["objects"][0]["m_indexBuffer"]["size"]   == 600);
        assert(j["header"]["bulkDataMap"]["0"]["size"]    == 400);
        assert(j["header"]["bulkDataMap"]["1"]["size"]    == 600);
    }

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir);
    auto loaded = db.LoadAsset(assetId);
    assert(loaded != nullptr);

    auto* lm = dynamic_cast<DTestMeshData*>(loaded->FindObject(meshId));
    assert(lm != nullptr);
    assert(lm->m_vertexCount == 100);
    assert(lm->m_indexCount  == 300);
    assert(lm->m_vertexBuffer.m_size == 400);
    for (int i = 0; i < 400; ++i)
        assert(lm->m_vertexBuffer.m_data[i] == static_cast<uint8_t>(i % 256));
    assert(lm->m_indexBuffer.m_size == 600);
    for (int i = 0; i < 600; ++i)
        assert(lm->m_indexBuffer.m_data[i] == static_cast<uint8_t>((i * 7) % 256));

    std::filesystem::remove_all(tempDir);
    std::cout << "[PASS] TestBulkDataRoundTrip\n";
}

// ---------------------------------------------------------------------------
// Test 10: Asset enumeration ignores bulk sidecars
// ---------------------------------------------------------------------------

static void TestAssetEnumerationIgnoresBulkFiles()
{
    auto tempDir = MakeTempDir("DeltaAssetEnumerationTest");

    AssetId assetId = UUID::Generate();
    nlohmann::json file = {
        {"header", {
            {"magic",     "DLTA"},
            {"version",   1},
            {"className", "PA_TestAsset"},
            {"assetId",   assetId.ToString()}
        }},
        {"objects", nlohmann::json::array()}
    };

    {
        std::ofstream(tempDir / "Enumerated.dasset.json") << file.dump(2);
        std::ofstream(tempDir / "Enumerated_Bulk0.bin", std::ios::binary) << "bulk";
    }

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir);

    const auto assets = db.GetAllAssets();
    assert(assets.size() == 1);
    assert(assets.begin()->first == assetId);
    assert(assets.begin()->second.m_filePath.filename() == "Enumerated.dasset.json");

    std::filesystem::remove_all(tempDir);
    std::cout << "[PASS] TestAssetEnumerationIgnoresBulkFiles\n";
}

// ---------------------------------------------------------------------------
// Test 11: SaveDirtyAssets persists changes and clears dirty
// ---------------------------------------------------------------------------

static void TestSaveDirtyAssetsPersistsChanges()
{
    auto tempDir = MakeTempDir("DeltaSaveDirtyAssetsTest");

    EditorAssetDatabase db;

    auto* asset = CreateDObject<PA_TestAsset>();
    asset->GetHeader().m_persistentId = UUID::Generate();
    asset->GetHeader().m_className = "PA_TestAsset";

    auto* rawObj = new DTestObjectA();
    rawObj->SetObjectId(UUID::Generate());
    rawObj->m_name = "BeforeSave";
    asset->AddObject(rawObj);

    const auto filePath = tempDir / "DirtyAsset.dasset.json";
    db.CreateAsset(filePath, asset);
    assert(!asset->IsDirty());

    DClass* dclass = rawObj->GetClass();
    assert(dclass != nullptr);
    DProperty* nameProp = dclass->FindPropertyByName("m_name");
    assert(nameProp != nullptr);

    const std::string newName = "AfterSave";
    nameProp->SetValue(rawObj, &newName);
    assert(asset->IsDirty());

    db.SaveDirtyAssets();
    assert(!asset->IsDirty());

    std::ifstream in(filePath);
    const auto json = nlohmann::json::parse(in);
    assert(json["objects"][0]["m_name"] == "AfterSave");
    in.close();

    std::filesystem::remove_all(tempDir);
    std::cout << "[PASS] TestSaveDirtyAssetsPersistsChanges\n";
}

// ---------------------------------------------------------------------------
// Test 12: DuplicateAsset remaps internal refs and preserves bulk/external refs
// ---------------------------------------------------------------------------

static void TestDuplicateAsset()
{
    auto tempDir = MakeTempDir("DeltaDuplicateAssetTest");

    auto* externalAsset = CreateDObject<PA_TestAsset>();
    const AssetId externalAssetId = UUID::Generate();
    externalAsset->GetHeader().m_persistentId = externalAssetId;
    externalAsset->GetHeader().m_className = "PA_TestAsset";

    auto* externalObj = new DTestObjectA();
    const ObjectId externalObjId = UUID::Generate();
    externalObj->SetObjectId(externalObjId);
    externalObj->m_name = "ExternalTarget";
    externalAsset->AddObject(externalObj);
    SaveAssetToFile(externalAsset, tempDir / "ExternalAsset.dasset.json");

    auto* sourceAsset = CreateDObject<PA_TestAsset>();
    const AssetId sourceAssetId = UUID::Generate();
    sourceAsset->GetHeader().m_persistentId = sourceAssetId;
    sourceAsset->GetHeader().m_className = "PA_TestAsset";

    auto* internalObj = new DTestObjectA();
    const ObjectId internalObjId = UUID::Generate();
    internalObj->SetObjectId(internalObjId);
    internalObj->m_name = "InternalTarget";
    sourceAsset->AddObject(internalObj);

    auto* refObj = new DTestObjectB();
    const ObjectId refObjId = UUID::Generate();
    refObj->SetObjectId(refObjId);
    refObj->m_label = "RefHolder";
    refObj->m_targetRef = internalObj;
    refObj->m_refs.push_back(internalObj);
    refObj->m_refs.push_back(externalObj);
    sourceAsset->AddObject(refObj);

    auto* meshObj = new DTestMeshData();
    meshObj->SetObjectId(UUID::Generate());
    meshObj->m_vertexCount = 2;
    meshObj->m_indexCount = 3;
    const uint8_t verts[] = { 1, 2, 3, 4 };
    const uint8_t indices[] = { 9, 8, 7, 6, 5, 4 };
    meshObj->m_vertexBuffer.Set(verts, sizeof(verts));
    meshObj->m_indexBuffer.Set(indices, sizeof(indices));
    sourceAsset->AddObject(meshObj);

    const auto sourceFile = tempDir / "SourceAsset.dasset.json";
    SaveAssetWithBulkData(sourceAsset, sourceFile);

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir);

    const AssetId duplicatedId = db.DuplicateAsset(sourceAssetId);
    assert(!duplicatedId.IsNull());
    assert(duplicatedId != sourceAssetId);

    DPrimaryAsset* duplicatedAsset = db.LoadAsset(duplicatedId);
    assert(duplicatedAsset != nullptr);
    assert(duplicatedAsset->GetObjects().size() == 3);

    DTestObjectA* duplicatedInternal = nullptr;
    DTestObjectB* duplicatedRef = nullptr;
    DTestMeshData* duplicatedMesh = nullptr;
    for (DObject* obj : duplicatedAsset->GetObjects())
    {
        if (!duplicatedInternal)
            duplicatedInternal = dynamic_cast<DTestObjectA*>(obj);
        if (!duplicatedRef)
            duplicatedRef = dynamic_cast<DTestObjectB*>(obj);
        if (!duplicatedMesh)
            duplicatedMesh = dynamic_cast<DTestMeshData*>(obj);
    }

    assert(duplicatedInternal != nullptr);
    assert(duplicatedRef != nullptr);
    assert(duplicatedMesh != nullptr);
    assert(duplicatedInternal->GetObjectId() != internalObjId);
    assert(duplicatedRef->GetObjectId() != refObjId);

    assert(duplicatedRef->m_targetRef == duplicatedInternal);
    assert(duplicatedRef->m_targetRef->GetOwningAsset()->GetAssetId() == duplicatedId);
    assert(duplicatedRef->m_refs.size() == 2);
    assert(duplicatedRef->m_refs[0] == duplicatedInternal);  // fixme error
    assert(duplicatedRef->m_refs[1] != nullptr);
    assert(duplicatedRef->m_refs[1]->GetObjectId() == externalObjId);
    assert(duplicatedRef->m_refs[1]->GetOwningAsset()->GetAssetId() == externalAssetId);

    assert(duplicatedMesh->m_vertexBuffer.m_size == sizeof(verts));
    assert(duplicatedMesh->m_indexBuffer.m_size == sizeof(indices));
    for (size_t i = 0; i < sizeof(verts); ++i)
        assert(duplicatedMesh->m_vertexBuffer.m_data[i] == verts[i]);
    for (size_t i = 0; i < sizeof(indices); ++i)
        assert(duplicatedMesh->m_indexBuffer.m_data[i] == indices[i]);

    const auto duplicatedPath = db.GetAssetPath(duplicatedId);
    assert(std::filesystem::exists(duplicatedPath));
    assert(std::filesystem::exists(duplicatedPath.parent_path() /
        (duplicatedPath.stem().stem().string() + "_Bulk0.bin")));
    assert(std::filesystem::exists(duplicatedPath.parent_path() /
        (duplicatedPath.stem().stem().string() + "_Bulk1.bin")));

    std::filesystem::remove_all(tempDir);
    std::cout << "[PASS] TestDuplicateAsset\n";
}

// ---------------------------------------------------------------------------
// Test 13: DeleteAsset removes json and bulk sidecars
// ---------------------------------------------------------------------------

static void TestDeleteAssetRemovesFiles()
{
    auto tempDir = MakeTempDir("DeltaDeleteAssetTest");

    auto* asset = CreateDObject<PA_TestMesh>();
    const AssetId assetId = UUID::Generate();
    asset->GetHeader().m_persistentId = assetId;
    asset->GetHeader().m_className = "PA_TestMesh";

    auto* meshObj = new DTestMeshData();
    meshObj->SetObjectId(UUID::Generate());
    const uint8_t bytes[] = { 1, 3, 5, 7 };
    meshObj->m_vertexBuffer.Set(bytes, sizeof(bytes));
    meshObj->m_indexBuffer.Set(bytes, sizeof(bytes));
    asset->AddObject(meshObj);

    const auto filePath = tempDir / "DeleteMe.dasset.json";
    SaveAssetWithBulkData(asset, filePath);

    EditorAssetDatabase db;
    db.ScanAssetsFolder(tempDir);
    assert(db.DeleteAsset(assetId));

    assert(!std::filesystem::exists(filePath));
    assert(!std::filesystem::exists(tempDir / "DeleteMe_Bulk0.bin"));
    assert(!std::filesystem::exists(tempDir / "DeleteMe_Bulk1.bin"));
    assert(db.FindAssetIdByPath(filePath).IsNull());
    assert(db.GetAllAssets().empty());

    std::filesystem::remove_all(tempDir);
    std::cout << "[PASS] TestDeleteAssetRemovesFiles\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main()
{
    GetReflectionRegistry().FinalizeRegistration();

    TestAssetDatabaseLocatorWithEditorAssetDatabase();
    TestSingleAssetRoundTrip();
    TestExtraDataIgnored();
    TestMissingDataUsesDefaults();
    TestUnknownClassSkipped();
    TestCrossAssetPointerResolution();
    TestCyclicDependency();
    TestDirtyFlagPropagation();
    TestBrokenReferenceIsNullptr();
    TestBulkDataRoundTrip();
    TestAssetEnumerationIgnoresBulkFiles();
    TestSaveDirtyAssetsPersistsChanges();
    TestDuplicateAsset();
    TestDeleteAssetRemovesFiles();

    std::cout << "\nAll serialization integration tests passed!\n";
    return 0;
}
