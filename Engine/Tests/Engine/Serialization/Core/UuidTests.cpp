#include "Runtime/Core/UUID.h"

#include <gtest/gtest.h>

#include <regex>
#include <unordered_map>
#include <unordered_set>

using namespace DeltaEngine;

TEST(UuidTests, RoundTripsToStringAndBack)
{
    const UUID id = UUID::Generate();

    EXPECT_FALSE(id.IsNull());
    EXPECT_EQ(UUID::FromString(id.ToString()), id);
}

TEST(UuidTests, NullUuidAndInvalidStringProduceNull)
{
    EXPECT_TRUE(UUID::Null().IsNull());
    EXPECT_TRUE(UUID::FromString("not-a-uuid").IsNull());
}

TEST(UuidTests, GeneratedValuesAreUniqueAcrossSample)
{
    std::unordered_set<UUID> ids;
    for (int index = 0; index < 1000; ++index)
        ids.insert(UUID::Generate());

    EXPECT_EQ(ids.size(), 1000u);
}

TEST(UuidTests, GeneratedValuesMatchExpectedFormat)
{
    const std::regex uuidRegex(
        "^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$");

    for (int index = 0; index < 100; ++index)
        EXPECT_TRUE(std::regex_match(UUID::Generate().ToString(), uuidRegex));
}

TEST(UuidTests, SupportsHashBasedContainers)
{
    std::unordered_map<UUID, int> values;
    const UUID key = UUID::Generate();

    values[key] = 42;

    EXPECT_EQ(values[key], 42);
}
