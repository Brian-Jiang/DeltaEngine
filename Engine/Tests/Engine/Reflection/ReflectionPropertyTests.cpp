#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Serialization/JsonAssetArchive.h"
#include "Runtime/Serialization/TBulkData.h"
#include "Runtime/Test/ReflectionTestObject.h"

#include <filesystem>
#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(ReflectionPropertyTests, StringField_RoundTripsUtf8Json)
{
    auto& registry = GetReflectionRegistry();
    DClass* cls = registry.FindClassByName("ReflectionTestObject");
    ASSERT_NE(cls, nullptr);
    ReflectionTestObject* obj =
        registry.CreateObject<ReflectionTestObject>("ReflectionTestObject");
    ASSERT_NE(obj, nullptr);

    DProperty* prop = cls->FindPropertyByName("m_rString");
    ASSERT_NE(prop, nullptr);

    std::string original = "caf\u00E9";
    prop->SetValue(obj, &original);

    JsonAssetArchive writer;
    prop->Serialize(writer, obj);

    std::string cleared;
    prop->SetValue(obj, &cleared);

    JsonAssetArchive reader(writer.GetRoot(), {});
    prop->Serialize(reader, obj);

    EXPECT_EQ(*static_cast<std::string*>(prop->GetValue(obj)), original);

    registry.DestroyObject(obj);
}

TEST(ReflectionPropertyTests, PathProperty_RoundTripsUnicodeSpacesAndNormalization)
{
    auto& registry = GetReflectionRegistry();
    DClass* cls = registry.FindClassByName("ReflectionTestObject");
    ASSERT_NE(cls, nullptr);
    ReflectionTestObject* obj =
        registry.CreateObject<ReflectionTestObject>("ReflectionTestObject");
    ASSERT_NE(obj, nullptr);

    DProperty* pathProp = cls->FindPropertyByName("m_rPath");
    ASSERT_NE(pathProp, nullptr);

    auto rel = std::filesystem::path("sub") / "\u6587caf\u00E9 with spaces.delta";
    const std::filesystem::path abs = std::filesystem::absolute(rel);

    pathProp->SetValue(obj, &abs);

    JsonAssetArchive w;
    pathProp->Serialize(w, obj);

    JsonAssetArchive r(w.GetRoot(), {});
    pathProp->Serialize(r, obj);

    void* pv = pathProp->GetValue(obj);
    ASSERT_NE(pv, nullptr);
    EXPECT_EQ(*static_cast<std::filesystem::path*>(pv), abs);

    registry.DestroyObject(obj);
}

TEST(ReflectionPropertyTests, PropertyInitializeDestroyCopy_FloatArrayStruct_Succeeds)
{
    auto& registry = GetReflectionRegistry();
    DClass* cls = registry.FindClassByName("ReflectionTestObject");
    ASSERT_NE(cls, nullptr);

    ReflectionTestObject* obj =
        registry.CreateObject<ReflectionTestObject>("ReflectionTestObject");
    ASSERT_NE(obj, nullptr);

    DProperty* floatProp = cls->FindPropertyByName("m_rFloat");
    DProperty* nestProp = cls->FindPropertyByName("m_rNest");

    ASSERT_NE(floatProp, nullptr);
    ASSERT_NE(nestProp, nullptr);

    float v = 2.25f;
    floatProp->SetValue(obj, &v);

    alignas(float) std::byte fStorage[sizeof(float)]{};
    floatProp->InitializeValue(static_cast<void*>(&fStorage[0]));
    floatProp->DestroyValue(static_cast<void*>(&fStorage[0]));

    alignas(ReflectionTestNestStruct) std::byte a[sizeof(ReflectionTestNestStruct)]{};
    alignas(ReflectionTestNestStruct) std::byte b[sizeof(ReflectionTestNestStruct)]{};
    nestProp->InitializeValue(static_cast<void*>(&a[0]));
    nestProp->CopyValue(static_cast<void*>(&b[0]), static_cast<void*>(&a[0]));
    nestProp->DestroyValue(static_cast<void*>(&b[0]));
    nestProp->DestroyValue(static_cast<void*>(&a[0]));

    registry.DestroyObject(obj);
}

TEST(ReflectionPropertyTests, StructProperty_SerializeJson_RoundTripsNestedFields)
{
    auto& registry = GetReflectionRegistry();
    DClass* cls = registry.FindClassByName("ReflectionTestObject");
    ASSERT_NE(cls, nullptr);
    ReflectionTestObject* obj =
        registry.CreateObject<ReflectionTestObject>("ReflectionTestObject");
    ASSERT_NE(obj, nullptr);

    DProperty* nestProp = cls->FindPropertyByName("m_rNest");
    ASSERT_NE(nestProp, nullptr);

    ReflectionTestNestStruct settings {};
    settings.m_nestedInt = 9;
    settings.m_nestedFloat = 0.25f;

    nestProp->SetValue(obj, &settings);

    JsonAssetArchive w;
    nestProp->Serialize(w, obj);

    settings.m_nestedInt = 0;
    nestProp->SetValue(obj, &settings);

    JsonAssetArchive r(w.GetRoot(), {});
    nestProp->Serialize(r, obj);

    void* sv = nestProp->GetValue(obj);
    ASSERT_NE(sv, nullptr);
    const auto& loaded = *static_cast<ReflectionTestNestStruct*>(sv);
    EXPECT_EQ(loaded.m_nestedInt, 9);
    EXPECT_FLOAT_EQ(loaded.m_nestedFloat, 0.25f);

    registry.DestroyObject(obj);
}

TEST(ReflectionPropertyTests, BulkData_SetValue_NullSource_ClearsField)
{
    auto& registry = GetReflectionRegistry();
    DClass* cls = registry.FindClassByName("ReflectionTestObject");
    ASSERT_NE(cls, nullptr);
    ReflectionTestObject* obj =
        registry.CreateObject<ReflectionTestObject>("ReflectionTestObject");
    ASSERT_NE(obj, nullptr);

    DProperty* bulkProp = cls->FindPropertyByName("m_rBulk");
    ASSERT_NE(bulkProp, nullptr);

    const std::byte bytes[] = { std::byte{0xAB}, std::byte{0xCD} };
    TBulkData src;
    src.Set(reinterpret_cast<const uint8_t*>(bytes), sizeof(bytes));
    src.m_bulkId = 42;

    bulkProp->SetValue(obj, &src);
    bulkProp->SetValue(obj, nullptr);

    void* bv = bulkProp->GetValue(obj);
    ASSERT_NE(bv, nullptr);
    EXPECT_FALSE(static_cast<TBulkData*>(bv)->IsValid());
    EXPECT_EQ(static_cast<TBulkData*>(bv)->m_bulkId, 0u);

    registry.DestroyObject(obj);
}

TEST(ReflectionPropertyTests, Regression_BulkClearPreviouslyLeftStaleHandle)
{
    auto& registry = GetReflectionRegistry();
    DClass* cls = registry.FindClassByName("ReflectionTestObject");
    ASSERT_NE(cls, nullptr);

    ReflectionTestObject* obj =
        registry.CreateObject<ReflectionTestObject>("ReflectionTestObject");
    ASSERT_NE(obj, nullptr);

    DProperty* bulkProp = cls->FindPropertyByName("m_rBulk");
    ASSERT_NE(bulkProp, nullptr);

    TBulkData nonEmpty;
    const uint8_t byte = 'x';
    nonEmpty.Set(&byte, 1);
    nonEmpty.m_bulkId = 9;

    bulkProp->SetValue(obj, &nonEmpty);
    bulkProp->SetValue(obj, nullptr);

    void* bv = bulkProp->GetValue(obj);
    ASSERT_NE(bv, nullptr);
    EXPECT_EQ(static_cast<TBulkData*>(bv)->m_bulkId, 0u);

    registry.DestroyObject(obj);
}
