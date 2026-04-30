#include "Runtime/Graphics/Shadow/ShadowMapAllocator.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(ShadowMapAllocatorTests, AllocateFailsWhenGridFull)
{
    ShadowMapAllocator a;
    a.Reset(1024, 1024, 1024);
    ShadowMapTileRegion r {};
    EXPECT_GE(a.Allocate(1024, r), 0);
    EXPECT_EQ(a.Allocate(1024, r), -1);
}

TEST(ShadowMapAllocatorTests, FreeAllowsReuseOfRegion)
{
    ShadowMapAllocator a;
    a.Reset(4096, 4096, 1024);
    ShadowMapTileRegion r0 {};
    const int32_t id0 = a.Allocate(2048, r0);
    ASSERT_GE(id0, 0);
    EXPECT_EQ(r0.x, 0u);
    EXPECT_EQ(r0.y, 0u);
    EXPECT_EQ(r0.width, 2048u);
    EXPECT_EQ(r0.height, 2048u);

    ShadowMapTileRegion r1 {};
    const int32_t id1 = a.Allocate(2048, r1);
    ASSERT_GE(id1, 0);

    a.Free(id0);
    ShadowMapTileRegion r2 {};
    const int32_t id2 = a.Allocate(2048, r2);
    ASSERT_GE(id2, 0);
    EXPECT_EQ(r2.x, r0.x);
    EXPECT_EQ(r2.y, r0.y);
    EXPECT_EQ(r2.width, r0.width);
    EXPECT_EQ(r2.height, r0.height);
}

TEST(ShadowMapAllocatorTests, FillAtlasUVRect)
{
    ShadowAllocation alloc {};
    ShadowMapTileRegion region { 1024, 512, 2048, 1024 };
    ShadowMapAllocator::FillAtlasUVRect(4096, 4096, region, alloc);
    EXPECT_FLOAT_EQ(alloc.atlasUVRect.x, 1024.f / 4096.f);
    EXPECT_FLOAT_EQ(alloc.atlasUVRect.y, 512.f / 4096.f);
    EXPECT_FLOAT_EQ(alloc.atlasUVRect.z, 2048.f / 4096.f);
    EXPECT_FLOAT_EQ(alloc.atlasUVRect.w, 1024.f / 4096.f);
}

TEST(PointSliceAllocatorTests, AllocateUntilFull)
{
    PointSliceAllocator p;
    p.Reset(2);
    EXPECT_EQ(p.Allocate(), 0);
    EXPECT_EQ(p.Allocate(), 1);
    EXPECT_EQ(p.Allocate(), -1);
}

TEST(PointSliceAllocatorTests, FreeAllowsAllocate)
{
    PointSliceAllocator p;
    p.Reset(3);
    EXPECT_EQ(p.Allocate(), 0);
    EXPECT_EQ(p.Allocate(), 1);
    p.Free(0);
    EXPECT_EQ(p.Allocate(), 0);
    EXPECT_EQ(p.Allocate(), 2);
    EXPECT_EQ(p.Allocate(), -1);
}
