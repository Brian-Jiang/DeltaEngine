#include "Runtime/Graphics/PostProcess/PA_PostProcessStack.h"
#include "Runtime/Graphics/PostProcess/PostProcessPass.h"
#include "Runtime/Graphics/PostProcess/PostProcessStack.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(PostProcessDataModelTests, CreateAssetProducesStackWithZeroPasses)
{
    auto* asset = PA_PostProcessStack::Create();
    ASSERT_NE(asset, nullptr);
    ASSERT_NE(asset->m_stack, nullptr);
    EXPECT_EQ(asset->m_stack->GetPassCount(), 0);
    EXPECT_EQ(asset->m_stack->GetPass(0), nullptr);
}

TEST(PostProcessDataModelTests, PostProcessPassClassIsAbstract)
{
    DClass* cls = GetReflectionRegistry().FindClassByName("PostProcessPass");
    ASSERT_NE(cls, nullptr);
    EXPECT_TRUE(cls->IsAbstract());
    EXPECT_EQ(GetReflectionRegistry().CreateObject("PostProcessPass"), nullptr);
}

TEST(PostProcessDataModelTests, PostProcessPassReflectedFields)
{
    DClass* cls = GetReflectionRegistry().FindClassByName("PostProcessPass");
    ASSERT_NE(cls, nullptr);
    EXPECT_NE(cls->FindPropertyByName("m_passName"), nullptr);
    EXPECT_NE(cls->FindPropertyByName("m_enabled"), nullptr);
}

TEST(PostProcessDataModelTests, PostProcessStackReflectedPasses)
{
    DClass* cls = GetReflectionRegistry().FindClassByName("PostProcessStack");
    ASSERT_NE(cls, nullptr);
    EXPECT_FALSE(cls->IsAbstract());
    EXPECT_NE(cls->FindPropertyByName("m_passes"), nullptr);
}

TEST(PostProcessDataModelTests, PA_PostProcessStackReflectedStack)
{
    DClass* cls = GetReflectionRegistry().FindClassByName("PA_PostProcessStack");
    ASSERT_NE(cls, nullptr);
    EXPECT_NE(cls->FindPropertyByName("m_stack"), nullptr);
}

TEST(PostProcessDataModelTests, AddPassAbstractClassReturnsNullptr)
{
    auto* asset = PA_PostProcessStack::Create();
    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(asset->AddPass("PostProcessPass"), nullptr);
    EXPECT_EQ(asset->m_stack->GetPassCount(), 0);
}

TEST(PostProcessDataModelTests, AddPassUnknownClassReturnsNullptr)
{
    auto* asset = PA_PostProcessStack::Create();
    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(asset->AddPass("NonReflectedPassType"), nullptr);
    EXPECT_EQ(asset->m_stack->GetPassCount(), 0);
}
