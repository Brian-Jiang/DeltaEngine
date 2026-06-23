#include "Runtime/Graphics/PostProcess/PA_PostProcessStack.h"
#include "Runtime/Graphics/PostProcess/ColorGradingPass.h"
#include "Runtime/Graphics/PostProcess/PassthroughPass.h"
#include "Runtime/Graphics/PostProcess/PostProcessPass.h"
#include "Runtime/Graphics/PostProcess/PostProcessStack.h"
#include "Runtime/Graphics/PostProcess/VignettePass.h"
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

TEST(PostProcessDataModelTests, PassthroughPassIsConcreteSubclassOfPostProcessPass)
{
    DObject* obj = GetReflectionRegistry().CreateObject("PassthroughPass");
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(obj->GetClass()->GetName(), "PassthroughPass");

    DClass* base = GetReflectionRegistry().FindClassByName("PostProcessPass");
    ASSERT_NE(base, nullptr);
    EXPECT_TRUE(obj->GetClass()->IsChildOf(base));

    GetReflectionRegistry().DestroyObject(obj);
}

TEST(PostProcessDataModelTests, ColorGradingPassIsConcreteSubclassOfPostProcessPass)
{
    DObject* obj = GetReflectionRegistry().CreateObject("ColorGradingPass");
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(obj->GetClass()->GetName(), "ColorGradingPass");

    DClass* base = GetReflectionRegistry().FindClassByName("PostProcessPass");
    ASSERT_NE(base, nullptr);
    EXPECT_TRUE(obj->GetClass()->IsChildOf(base));

    DClass* cls = obj->GetClass();
    EXPECT_NE(cls->FindPropertyByName("m_saturation"), nullptr);
    EXPECT_NE(cls->FindPropertyByName("m_contrast"), nullptr);
    EXPECT_NE(cls->FindPropertyByName("m_lift"), nullptr);
    EXPECT_NE(cls->FindPropertyByName("m_gamma"), nullptr);
    EXPECT_NE(cls->FindPropertyByName("m_gain"), nullptr);

    GetReflectionRegistry().DestroyObject(obj);
}

TEST(PostProcessDataModelTests, VignettePassIsConcreteSubclassOfPostProcessPass)
{
    DObject* obj = GetReflectionRegistry().CreateObject("VignettePass");
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(obj->GetClass()->GetName(), "VignettePass");

    DClass* base = GetReflectionRegistry().FindClassByName("PostProcessPass");
    ASSERT_NE(base, nullptr);
    EXPECT_TRUE(obj->GetClass()->IsChildOf(base));

    DClass* cls = obj->GetClass();
    EXPECT_NE(cls->FindPropertyByName("m_intensity"), nullptr);
    EXPECT_NE(cls->FindPropertyByName("m_smoothness"), nullptr);
    EXPECT_NE(cls->FindPropertyByName("m_roundness"), nullptr);
    EXPECT_NE(cls->FindPropertyByName("m_color"), nullptr);

    GetReflectionRegistry().DestroyObject(obj);
}
