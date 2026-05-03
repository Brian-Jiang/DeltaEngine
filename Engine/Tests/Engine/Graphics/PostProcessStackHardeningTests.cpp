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
    ASSERT_NE(asset->m_stack, nullptr);

    const int countBefore = asset->m_stack->GetPassCount();
    PostProcessPass* added = asset->AddPass("PassthroughPass");
    ASSERT_NE(added, nullptr);
    EXPECT_EQ(asset->m_stack->GetPassCount(), countBefore + 1);

    PostProcessPass* p0 = asset->m_stack->GetPass(0);
    EXPECT_EQ(p0, added);
    EXPECT_NE(dynamic_cast<PassthroughPass*>(p0), nullptr);

    GetReflectionRegistry().DestroyObject(asset);
}

TEST(PostProcessStackHardeningTests, PostProcessStack_GetPass_NegativeIndex_ReturnsNull)
{
    PA_PostProcessStack* asset = PA_PostProcessStack::Create();
    ASSERT_NE(asset, nullptr);
    ASSERT_NE(asset->m_stack, nullptr);

    EXPECT_EQ(asset->m_stack->GetPass(-1), nullptr);

    GetReflectionRegistry().DestroyObject(asset);
}

TEST(PostProcessStackHardeningTests, PostProcessStack_GetPass_OutOfRange_ReturnsNull)
{
    PA_PostProcessStack* asset = PA_PostProcessStack::Create();
    ASSERT_NE(asset, nullptr);
    ASSERT_NE(asset->m_stack, nullptr);
    ASSERT_NE(asset->AddPass("PassthroughPass"), nullptr);

    EXPECT_EQ(asset->m_stack->GetPass(1), nullptr);

    GetReflectionRegistry().DestroyObject(asset);
}

TEST(PostProcessStackHardeningTests, PA_PostProcessStack_AddPass_ValidClass_IncrementsPassCount)
{
    PA_PostProcessStack* asset = PA_PostProcessStack::Create();
    ASSERT_NE(asset, nullptr);
    PostProcessStack* stack = asset->m_stack;
    ASSERT_NE(stack, nullptr);

    EXPECT_EQ(stack->GetPassCount(), 0);
    ASSERT_NE(asset->AddPass("PassthroughPass"), nullptr);
    EXPECT_EQ(stack->GetPassCount(), 1);
    ASSERT_NE(asset->AddPass("TonemapPass"), nullptr);
    EXPECT_EQ(stack->GetPassCount(), 2);

    GetReflectionRegistry().DestroyObject(asset);
}
