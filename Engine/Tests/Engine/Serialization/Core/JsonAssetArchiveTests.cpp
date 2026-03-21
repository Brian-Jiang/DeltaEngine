#include "Runtime/Serialization/JsonAssetArchive.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(JsonAssetArchiveTests, RoundTripsWideStringsAsUtf8)
{
    JsonAssetArchive writer;
    std::wstring expected = L"Delta caf\u00E9 \u4F60\u597D";

    writer.Serialize("label", expected);

    JsonAssetArchive reader(writer.GetRoot(), {});
    std::wstring loaded;
    reader.Serialize("label", loaded);

    EXPECT_EQ(loaded, expected);
}

TEST(JsonAssetArchiveTests, RoundTripsNestedObjectsAndArrays)
{
    JsonAssetArchive writer;

    writer.BeginNestedObject("outer");

    int count = 3;
    writer.Serialize("count", count);

    writer.BeginArray("items", 2);
    int firstValue = 11;
    writer.SerializeElement(firstValue);

    writer.BeginNestedArray(2);
    float x = 1.5f;
    float y = 2.5f;
    writer.SerializeElement(x);
    writer.SerializeElement(y);
    writer.EndArray();
    writer.EndArray();
    writer.EndNestedObject();

    JsonAssetArchive reader(writer.GetRoot(), {});
    ASSERT_TRUE(reader.BeginNestedObjectLoad("outer"));

    int loadedCount = 0;
    reader.Serialize("count", loadedCount);
    EXPECT_EQ(loadedCount, 3);

    const size_t itemCount = reader.BeginArrayLoad("items");
    ASSERT_EQ(itemCount, 2u);

    int loadedFirstValue = 0;
    reader.SerializeElement(loadedFirstValue);
    EXPECT_EQ(loadedFirstValue, 11);

    const size_t nestedCount = reader.BeginNestedArrayLoad();
    ASSERT_EQ(nestedCount, 2u);

    float loadedX = 0.0f;
    float loadedY = 0.0f;
    reader.SerializeElement(loadedX);
    reader.SerializeElement(loadedY);
    EXPECT_FLOAT_EQ(loadedX, 1.5f);
    EXPECT_FLOAT_EQ(loadedY, 2.5f);

    reader.EndArray();
    reader.EndArray();
    reader.EndNestedObject();
}
