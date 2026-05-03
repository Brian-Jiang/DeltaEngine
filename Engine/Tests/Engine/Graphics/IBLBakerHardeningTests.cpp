#include "Runtime/Graphics/IBL/IBLBaker.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(IBLBakerHardeningTests, IBLBaker_DefaultConstruction_StaticLutIsZero)
{
    // Arrange / Act
    IBLBaker baker;
    const IBLBaker::IBLResources& staticLut = baker.GetStaticLut();

    // Assert
    EXPECT_EQ(staticLut.brdfLut, nullptr);
    EXPECT_EQ(staticLut.irradianceCube, nullptr);
    EXPECT_EQ(staticLut.specularCube, nullptr);
    EXPECT_EQ(staticLut.brdfLutSRV.ptr, 0u);
    EXPECT_EQ(staticLut.irradianceSRV.ptr, 0u);
    EXPECT_EQ(staticLut.specularSRV.ptr, 0u);
}

TEST(IBLBakerHardeningTests, IBLBaker_Shutdown_BeforeInitialize_DoesNotCrash)
{
    // Arrange
    IBLBaker baker;

    // Act / Assert (no crash)
    baker.Shutdown();
    SUCCEED();
}

TEST(IBLBakerHardeningTests, IBLBaker_Shutdown_CalledTwice_IsIdempotent)
{
    // Arrange
    IBLBaker baker;

    // Act
    baker.Shutdown();
    baker.Shutdown();

    // Assert
    EXPECT_EQ(baker.GetStaticLut().brdfLut, nullptr);
}
