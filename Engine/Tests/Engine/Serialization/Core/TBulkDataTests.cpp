#include "Runtime/Serialization/TBulkData.h"

#include <gtest/gtest.h>

#include <array>
#include <utility>

using namespace DeltaEngine;

TEST(TBulkDataTests, DefaultsToAnUnloadedState)
{
    TBulkData bulkData;

    EXPECT_FALSE(bulkData.IsValid());
    EXPECT_EQ(bulkData.m_data, nullptr);
    EXPECT_EQ(bulkData.m_size, 0u);
    EXPECT_EQ(bulkData.m_bulkId, 0u);
}

TEST(TBulkDataTests, SetCopiesPayloadAndCopyConstructorMakesIndependentStorage)
{
    const std::array<uint8_t, 4> sourceBytes{ 1, 2, 3, 4 };

    TBulkData original;
    original.m_bulkId = 12;
    original.Set(sourceBytes.data(), sourceBytes.size());

    TBulkData copy(original);

    ASSERT_NE(copy.m_data, nullptr);
    EXPECT_NE(copy.m_data, original.m_data);
    EXPECT_EQ(copy.m_size, original.m_size);
    EXPECT_EQ(copy.m_bulkId, original.m_bulkId);
    EXPECT_EQ(copy.m_data[0], 1u);

    original.m_data[0] = 99;
    EXPECT_EQ(copy.m_data[0], 1u);
}

TEST(TBulkDataTests, MoveTransfersPayloadOwnership)
{
    const std::array<uint8_t, 3> sourceBytes{ 8, 9, 10 };

    TBulkData original;
    original.m_bulkId = 7;
    original.Set(sourceBytes.data(), sourceBytes.size());
    uint8_t* originalData = original.m_data;

    TBulkData moved(std::move(original));

    EXPECT_EQ(moved.m_data, originalData);
    EXPECT_EQ(moved.m_size, sourceBytes.size());
    EXPECT_EQ(moved.m_bulkId, 7u);
    EXPECT_EQ(original.m_data, nullptr);
    EXPECT_EQ(original.m_size, 0u);
    EXPECT_EQ(original.m_bulkId, 0u);
}

TEST(TBulkDataTests, ConvertsToAndFromHandles)
{
    const BulkDataHandle handle{ 42u, 512u };

    TBulkData bulkData;
    bulkData.ApplyHandle(handle);

    EXPECT_FALSE(bulkData.IsValid());
    EXPECT_EQ(bulkData.m_bulkId, 42u);
    EXPECT_EQ(bulkData.m_size, 512u);

    const BulkDataHandle roundTrip = bulkData.ToHandle();
    EXPECT_EQ(roundTrip, handle);
}
