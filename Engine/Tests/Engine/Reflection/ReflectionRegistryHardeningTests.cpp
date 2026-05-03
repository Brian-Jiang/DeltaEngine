#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Reflection/DStruct.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(ReflectionRegistryHardeningTests, FindStructByName_ReflectionTestNestStruct_ReturnsSchema)
{
    auto& registry = GetReflectionRegistry();

    DStruct* nest = registry.FindStructByName("ReflectionTestNestStruct");

    ASSERT_NE(nest, nullptr);
    EXPECT_EQ(nest->GetName(), "ReflectionTestNestStruct");
}

TEST(ReflectionRegistryHardeningTests, FindStructByName_ShadowSettings_ReturnsSchema)
{
    auto& registry = GetReflectionRegistry();

    DStruct* shadowStruct = registry.FindStructByName("ShadowSettings");

    ASSERT_NE(shadowStruct, nullptr);
    EXPECT_EQ(shadowStruct->GetName(), "ShadowSettings");
}

TEST(ReflectionRegistryHardeningTests, FindStructByName_Empty_ReturnsNull)
{
    auto& registry = GetReflectionRegistry();

    DStruct* ds = registry.FindStructByName("");

    EXPECT_EQ(ds, nullptr);
}

TEST(ReflectionRegistryHardeningTests, FindClassByName_Empty_ReturnsNull)
{
    auto& registry = GetReflectionRegistry();

    DClass* cls = registry.FindClassByName("");

    EXPECT_EQ(cls, nullptr);
}

TEST(ReflectionRegistryHardeningTests, GetAllClasses_ContainsConcreteTestTypes)
{
    auto& registry = GetReflectionRegistry();
    const auto& classes = registry.GetAllClasses();

    ASSERT_FALSE(classes.empty());
    EXPECT_NE(classes.find("TestComponent"), classes.end());
    EXPECT_NE(classes.find("ReflectionTestObject"), classes.end());
}

TEST(ReflectionRegistryHardeningTests, CreateObject_Unknown_ReturnsNull)
{
    auto& registry = GetReflectionRegistry();

    DObject* obj = registry.CreateObject("NonexistentReflectClass_xyz");

    EXPECT_EQ(obj, nullptr);
}

TEST(ReflectionRegistryHardeningTests, CreateObject_EmptyName_ReturnsNull)
{
    auto& registry = GetReflectionRegistry();

    DObject* obj = registry.CreateObject("");

    EXPECT_EQ(obj, nullptr);
}

TEST(ReflectionRegistryHardeningTests, CreateObject_AbstractPostProcessPass_ReturnsNull)
{
    auto& registry = GetReflectionRegistry();

    DObject* obj = registry.CreateObject("PostProcessPass");

    EXPECT_EQ(obj, nullptr);
}

TEST(ReflectionRegistryHardeningTests, DestroyObject_Null_IsNoOp)
{
    auto& registry = GetReflectionRegistry();

    registry.DestroyObject(nullptr);
}
