#include "Runtime/Graphics/Shadow/ShadowMapAllocator.h"

#include <gtest/gtest.h>
#include <tuple>
#include <cmath>

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

TEST(ShadowMapAllocatorTests, FreeReusesHoleAfterFragmentation)
{
    ShadowMapAllocator a;
    a.Reset(2048, 2048, 512);
    ShadowMapTileRegion r0 {}, r1 {}, r2 {}, r3 {};
    const int32_t id0 = a.Allocate(1024, r0);
    const int32_t id1 = a.Allocate(1024, r1);
    const int32_t id2 = a.Allocate(1024, r2);
    const int32_t id3 = a.Allocate(1024, r3);
    ASSERT_GE(id0, 0);
    ASSERT_GE(id1, 0);
    ASSERT_GE(id2, 0);
    ASSERT_GE(id3, 0);
    EXPECT_EQ(a.Allocate(1024, r0), -1);

    const uint32_t freedX = r1.x;
    const uint32_t freedY = r1.y;
    a.Free(id1);
    ShadowMapTileRegion rReuse {};
    const int32_t id4 = a.Allocate(1024, rReuse);
    ASSERT_GE(id4, 0);
    EXPECT_EQ(rReuse.x, freedX);
    EXPECT_EQ(rReuse.y, freedY);
    EXPECT_EQ(rReuse.width, 1024u);
    EXPECT_EQ(rReuse.height, 1024u);
    EXPECT_GT(id4, id3);
}

TEST(ShadowMapAllocatorTests, RepeatedSequenceIsDeterministicWithMonotonicSlotIds)
{
    auto run = [] {
        ShadowMapAllocator a;
        a.Reset(4096, 4096, 1024);
        ShadowMapTileRegion ra {}, rb {}, rc {};
        const int32_t id0 = a.Allocate(2048, ra);
        const int32_t id1 = a.Allocate(2048, rb);
        EXPECT_EQ(id0, 0);
        EXPECT_EQ(id1, 1);
        a.Free(id0);
        const int32_t id2 = a.Allocate(2048, rc);
        EXPECT_EQ(id2, 2);
        return std::tuple { ra, rb, rc, id0, id1, id2 };
    };
    const auto t0 = run();
    const auto t1 = run();
    EXPECT_EQ(std::get<0>(t0).x, std::get<0>(t1).x);
    EXPECT_EQ(std::get<1>(t0).x, std::get<1>(t1).x);
    EXPECT_EQ(std::get<2>(t0).x, std::get<2>(t1).x);
    EXPECT_EQ(std::get<2>(t0).y, std::get<2>(t1).y);
}

TEST(ShadowMapAllocatorTests, AllocatesExpectedNumberOfMinimalTiles)
{
    ShadowMapAllocator a;
    a.Reset(1024, 1024, 512);
    int count = 0;
    ShadowMapTileRegion r {};
    while (a.Allocate(512, r) >= 0)
        ++count;
    EXPECT_EQ(count, 4);
}

TEST(ShadowMapAllocatorTests, Free_UnknownSlot_LeavesOccupancyUnchanged)
{
    ShadowMapAllocator a;
    a.Reset(1024, 1024, 512);
    ShadowMapTileRegion r0 {};
    ShadowMapTileRegion r1 {};
    const int32_t id0 = a.Allocate(512, r0);
    const int32_t id1 = a.Allocate(512, r1);
    ASSERT_GE(id0, 0);
    ASSERT_GE(id1, 0);

    a.Free(99999);

    ShadowMapTileRegion r2 {};
    EXPECT_EQ(a.Allocate(1024, r2), -1);
}

TEST(ShadowMapAllocatorTests, FillAtlasUVRect_ZeroRegion_YieldsFiniteUVs)
{
    ShadowAllocation alloc {};
    ShadowMapTileRegion region { 0, 0, 0, 0 };
    ShadowMapAllocator::FillAtlasUVRect(1, 1, region, alloc);
    EXPECT_TRUE(std::isfinite(alloc.atlasUVRect.x));
    EXPECT_TRUE(std::isfinite(alloc.atlasUVRect.y));
    EXPECT_TRUE(std::isfinite(alloc.atlasUVRect.z));
    EXPECT_TRUE(std::isfinite(alloc.atlasUVRect.w));
    EXPECT_FLOAT_EQ(alloc.atlasUVRect.x, 0.0f);
    EXPECT_FLOAT_EQ(alloc.atlasUVRect.y, 0.0f);
    EXPECT_FLOAT_EQ(alloc.atlasUVRect.z, 0.0f);
    EXPECT_FLOAT_EQ(alloc.atlasUVRect.w, 0.0f);
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

TEST(PointSliceAllocatorTests, Reset_ZeroMaxCubes_AllocateReturnsNegative)
{
    PointSliceAllocator p;
    p.Reset(0);
    EXPECT_EQ(p.Allocate(), -1);
}
