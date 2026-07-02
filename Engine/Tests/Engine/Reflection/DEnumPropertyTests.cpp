#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DEnumProperty.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Serialization/JsonAssetArchive.h"
#include "Runtime/Test/ReflectionTestObject.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{

DProperty* GetReflectionTestEnumProperty()
{
    DClass* cls = GetReflectionRegistry().FindClassByName("ReflectionTestObject");
    if (cls == nullptr)
        return nullptr;
    return cls->FindPropertyByName("m_rEnum");
}

}

TEST(DEnumPropertyTests, FindEnumByName_ReturnsRegisteredMetadata)
{
    DEnum* denum = GetReflectionRegistry().FindEnumByName("EReflectionTestEnum");
    ASSERT_NE(denum, nullptr);
    EXPECT_EQ(denum->GetName(), "EReflectionTestEnum");
    EXPECT_EQ(denum->GetUnderlyingType(), "uint32_t");

    const std::vector<DEnumEntry>& entries = denum->GetEntries();
    ASSERT_EQ(entries.size(), 3u);
    EXPECT_EQ(entries[0].name, "Foo");
    EXPECT_EQ(entries[0].value, 0);
    EXPECT_EQ(entries[1].name, "Bar");
    EXPECT_EQ(entries[1].value, 1);
    EXPECT_EQ(entries[2].name, "Baz");
    EXPECT_EQ(entries[2].value, 2);
}

TEST(DEnumPropertyTests, GetPropertyType_IsEnum)
{
    DProperty* prop = GetReflectionTestEnumProperty();
    ASSERT_NE(prop, nullptr);
    EXPECT_EQ(prop->GetPropertyType(), EPropertyType::Enum);
}

TEST(DEnumPropertyTests, GetEnum_ResolvesFromProperty)
{
    DProperty* prop = GetReflectionTestEnumProperty();
    ASSERT_NE(prop, nullptr);
    auto* enumProp = static_cast<DEnumPropertyBase*>(prop);
    DEnum* fromProperty = enumProp->GetEnumSchema();
    DEnum* fromRegistry = GetReflectionRegistry().FindEnumByName("EReflectionTestEnum");
    EXPECT_EQ(fromProperty, fromRegistry);
}

TEST(DEnumPropertyTests, RoundTripsIntegerJson)
{
    auto& registry = GetReflectionRegistry();
    ReflectionTestObject* obj =
        registry.CreateObject<ReflectionTestObject>("ReflectionTestObject");
    ASSERT_NE(obj, nullptr);

    DProperty* prop = GetReflectionTestEnumProperty();
    ASSERT_NE(prop, nullptr);

    EReflectionTestEnum value = EReflectionTestEnum::Bar;
    prop->SetValue(obj, &value);

    JsonAssetArchive writer;
    prop->Serialize(writer, obj);

    const nlohmann::json& root = writer.GetRoot();
    ASSERT_TRUE(root.contains("m_rEnum"));
    EXPECT_EQ(root["m_rEnum"].get<int>(), 1);

    EReflectionTestEnum reset = EReflectionTestEnum::Foo;
    prop->SetValue(obj, &reset);

    JsonAssetArchive reader(root, {});
    prop->Serialize(reader, obj);

    EXPECT_EQ(*static_cast<EReflectionTestEnum*>(prop->GetValue(obj)), EReflectionTestEnum::Bar);

    registry.DestroyObject(obj);
}

TEST(DEnumPropertyTests, Identical_SameValue)
{
    DProperty* prop = GetReflectionTestEnumProperty();
    ASSERT_NE(prop, nullptr);

    EReflectionTestEnum a = EReflectionTestEnum::Bar;
    EReflectionTestEnum b = EReflectionTestEnum::Bar;
    EXPECT_TRUE(prop->Identical(&a, &b));
}

TEST(DEnumPropertyTests, Identical_DifferentValue)
{
    DProperty* prop = GetReflectionTestEnumProperty();
    ASSERT_NE(prop, nullptr);

    EReflectionTestEnum a = EReflectionTestEnum::Foo;
    EReflectionTestEnum b = EReflectionTestEnum::Baz;
    EXPECT_FALSE(prop->Identical(&a, &b));
}

TEST(DEnumPropertyTests, ToString_KnownEnumerator)
{
    DProperty* prop = GetReflectionTestEnumProperty();
    ASSERT_NE(prop, nullptr);

    EReflectionTestEnum value = EReflectionTestEnum::Bar;
    EXPECT_EQ(prop->ToString(&value), "Bar");
}

TEST(DEnumPropertyTests, ToString_UnknownValue)
{
    DProperty* prop = GetReflectionTestEnumProperty();
    ASSERT_NE(prop, nullptr);

    EReflectionTestEnum value = static_cast<EReflectionTestEnum>(99);
    EXPECT_EQ(prop->ToString(&value), "99");
}

TEST(DEnumPropertyTests, MissingKey_LeavesValueUntouched)
{
    auto& registry = GetReflectionRegistry();
    ReflectionTestObject* obj =
        registry.CreateObject<ReflectionTestObject>("ReflectionTestObject");
    ASSERT_NE(obj, nullptr);

    DProperty* prop = GetReflectionTestEnumProperty();
    ASSERT_NE(prop, nullptr);

    EReflectionTestEnum value = EReflectionTestEnum::Baz;
    prop->SetValue(obj, &value);

    const nlohmann::json root = nlohmann::json::object();
    JsonAssetArchive reader(root, {});
    prop->Serialize(reader, obj);

    EXPECT_EQ(*static_cast<EReflectionTestEnum*>(prop->GetValue(obj)), EReflectionTestEnum::Baz);

    registry.DestroyObject(obj);
}
