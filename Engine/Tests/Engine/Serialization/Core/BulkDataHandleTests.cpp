#include "Runtime/Serialization/BulkDataHandle.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(BulkDataHandleTests, BecomesValidWhenSizeIsSet)
{
    BulkDataHandle handle;
    EXPECT_FALSE(handle.IsValid());

    handle.m_dataSize = 1024;

    EXPECT_TRUE(handle.IsValid());
}
