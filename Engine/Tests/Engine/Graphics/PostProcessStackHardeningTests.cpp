#include "Runtime/Graphics/PostProcess/BloomPass.h"
#include "Runtime/Graphics/PostProcess/PA_PostProcessStack.h"
#include "Runtime/Graphics/PostProcess/PassthroughPass.h"
#include "Runtime/Graphics/PostProcess/PostProcessStack.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(PostProcessStackHardeningTests, PostProcessStack_GetPass_ValidIndex_ReturnsPass)
{
    PA_PostProcessStack* asset = PA_PostProcessStack::Create();
    ASSERT_NE(asset, nullptr);
    ASSERT_NE(asset->GetStack(), nullptr);

    const int countBefore = asset->GetStack()->GetPassCount();
    PostProcessPass* added = asset->AddPass("PassthroughPass");
    ASSERT_NE(added, nullptr);
    EXPECT_EQ(asset->GetStack()->GetPassCount(), countBefore + 1);

    PostProcessPass* p0 = asset->GetStack()->GetPass(0);
    EXPECT_EQ(p0, added);
    EXPECT_NE(dynamic_cast<PassthroughPass*>(p0), nullptr);

    GetReflectionRegistry().DestroyObject(asset);
}

TEST(PostProcessStackHardeningTests, PostProcessStack_GetPass_NegativeIndex_ReturnsNull)
{
    PA_PostProcessStack* asset = PA_PostProcessStack::Create();
    ASSERT_NE(asset, nullptr);
    ASSERT_NE(asset->GetStack(), nullptr);

    EXPECT_EQ(asset->GetStack()->GetPass(-1), nullptr);

    GetReflectionRegistry().DestroyObject(asset);
}

TEST(PostProcessStackHardeningTests, PostProcessStack_GetPass_OutOfRange_ReturnsNull)
{
    PA_PostProcessStack* asset = PA_PostProcessStack::Create();
    ASSERT_NE(asset, nullptr);
    ASSERT_NE(asset->GetStack(), nullptr);
    ASSERT_NE(asset->AddPass("PassthroughPass"), nullptr);

    EXPECT_EQ(asset->GetStack()->GetPass(1), nullptr);

    GetReflectionRegistry().DestroyObject(asset);
}

TEST(PostProcessStackHardeningTests, PA_PostProcessStack_AddPass_ValidClass_IncrementsPassCount)
{
    PA_PostProcessStack* asset = PA_PostProcessStack::Create();
    ASSERT_NE(asset, nullptr);
    PostProcessStack* stack = asset->GetStack();
    ASSERT_NE(stack, nullptr);

    EXPECT_EQ(stack->GetPassCount(), 0);
    ASSERT_NE(asset->AddPass("PassthroughPass"), nullptr);
    EXPECT_EQ(stack->GetPassCount(), 1);
    ASSERT_NE(asset->AddPass("TonemapPass"), nullptr);
    EXPECT_EQ(stack->GetPassCount(), 2);
    ASSERT_NE(asset->AddPass("ColorGradingPass"), nullptr);
    EXPECT_EQ(stack->GetPassCount(), 3);
    ASSERT_NE(asset->AddPass("VignettePass"), nullptr);
    EXPECT_EQ(stack->GetPassCount(), 4);

    GetReflectionRegistry().DestroyObject(asset);
}

TEST(PostProcessStackHardeningTests, PA_PostProcessStack_AddBloomPass_IncrementsPassCount)
{
    PA_PostProcessStack* asset = PA_PostProcessStack::Create();
    ASSERT_NE(asset, nullptr);
    PostProcessStack* stack = asset->GetStack();
    ASSERT_NE(stack, nullptr);

    EXPECT_EQ(stack->GetPassCount(), 0);
    PostProcessPass* bloom = asset->AddPass("BloomPass");
    ASSERT_NE(bloom, nullptr);
    EXPECT_EQ(stack->GetPassCount(), 1);
    EXPECT_NE(dynamic_cast<BloomPass*>(bloom), nullptr);

    GetReflectionRegistry().DestroyObject(asset);
}
