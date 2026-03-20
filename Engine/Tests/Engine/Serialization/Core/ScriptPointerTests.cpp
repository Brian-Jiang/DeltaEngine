#include "Runtime/Core/UUID.h"
#include "Runtime/Serialization/ScriptPointer.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(ScriptPointerTests, ReportsNullAndExternalState)
{
    ScriptPointer pointer;
    EXPECT_TRUE(pointer.IsNull());

    pointer.m_assetId = UUID::Generate();
    pointer.m_objectId = UUID::Generate();

    const AssetId otherAssetId = UUID::Generate();

    EXPECT_FALSE(pointer.IsNull());
    EXPECT_TRUE(pointer.IsExternal(otherAssetId));
    EXPECT_FALSE(pointer.IsExternal(pointer.m_assetId));
}
