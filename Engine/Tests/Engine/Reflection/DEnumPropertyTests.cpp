#include "Runtime/Macros.h"
#include "Runtime/Reflection/DEnum.h"
#include "Runtime/Reflection/DEnumProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Serialization/JsonAssetArchive.h"

#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{

DENUM()
enum class EReflectionTestEnum : uint32_t
{
    Foo = 0,
    Bar = 1,
    Baz = 2,
};

static void RegisterReflectionTestEnum()
{
    auto* e = new DEnum("EReflectionTestEnum", "uint32_t");
    e->AddEntry("Foo", 0);
    e->AddEntry("Bar", 1);
    e->AddEntry("Baz", 2);
    GetReflectionRegistry().RegisterDEnum(e);
}

static ReflectionRegistration s_reg(&RegisterReflectionTestEnum);

struct FEnumPropertyTestPayload
{
    EReflectionTestEnum m_mode{ EReflectionTestEnum::Foo };
};

static DEnumProperty<EReflectionTestEnum> s_enumProperty(
    "m_mode",
    offsetof(FEnumPropertyTestPayload, m_mode),
    "EReflectionTestEnum");

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
    EXPECT_EQ(s_enumProperty.GetPropertyType(), EPropertyType::Enum);
}

TEST(DEnumPropertyTests, GetEnum_ResolvesFromProperty)
{
    DEnum* fromProperty = s_enumProperty.GetEnum();
    DEnum* fromRegistry = GetReflectionRegistry().FindEnumByName("EReflectionTestEnum");
    EXPECT_EQ(fromProperty, fromRegistry);
}

TEST(DEnumPropertyTests, RoundTripsIntegerJson)
{
    FEnumPropertyTestPayload payload;
    payload.m_mode = EReflectionTestEnum::Bar;

    JsonAssetArchive writer;
    s_enumProperty.Serialize(writer, &payload);

    const nlohmann::json& root = writer.GetRoot();
    ASSERT_TRUE(root.contains("m_mode"));
    EXPECT_EQ(root["m_mode"].get<int>(), 1);

    payload.m_mode = EReflectionTestEnum::Foo;

    JsonAssetArchive reader(root, {});
    s_enumProperty.Serialize(reader, &payload);

    EXPECT_EQ(payload.m_mode, EReflectionTestEnum::Bar);
}

TEST(DEnumPropertyTests, Identical_SameValue)
{
    EReflectionTestEnum a = EReflectionTestEnum::Bar;
    EReflectionTestEnum b = EReflectionTestEnum::Bar;
    EXPECT_TRUE(s_enumProperty.Identical(&a, &b));
}

TEST(DEnumPropertyTests, Identical_DifferentValue)
{
    EReflectionTestEnum a = EReflectionTestEnum::Foo;
    EReflectionTestEnum b = EReflectionTestEnum::Baz;
    EXPECT_FALSE(s_enumProperty.Identical(&a, &b));
}

TEST(DEnumPropertyTests, ToString_KnownEnumerator)
{
    EReflectionTestEnum value = EReflectionTestEnum::Bar;
    EXPECT_EQ(s_enumProperty.ToString(&value), "Bar");
}

TEST(DEnumPropertyTests, ToString_UnknownValue)
{
    EReflectionTestEnum value = static_cast<EReflectionTestEnum>(99);
    EXPECT_EQ(s_enumProperty.ToString(&value), "99");
}

TEST(DEnumPropertyTests, MissingKey_LeavesValueUntouched)
{
    FEnumPropertyTestPayload payload;
    payload.m_mode = EReflectionTestEnum::Baz;

    const nlohmann::json root = nlohmann::json::object();
    JsonAssetArchive reader(root, {});
    s_enumProperty.Serialize(reader, &payload);

    EXPECT_EQ(payload.m_mode, EReflectionTestEnum::Baz);
}
