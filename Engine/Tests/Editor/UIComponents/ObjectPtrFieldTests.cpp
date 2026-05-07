#include "Editor/UIComponents/UIComponentsImGuiFixture.h"

#include "Editor/UIComponents/PropertyWidgets/ObjectPtrField.h"

#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

TEST_F(UIComponentsObjectPtrFixture, ObjectPtrField_Draw_NullLabel_ReturnsNullOpt)
{
    ObjectPtrField f;
    DClass*        go = GetReflectionRegistry().FindClassByName("GameObject");
    ASSERT_NE(go, nullptr);

    EXPECT_FALSE(f.Draw(nullptr, nullptr, go, "##p").has_value());
}

TEST_F(UIComponentsObjectPtrFixture, ObjectPtrField_Draw_NullTargetClass_ReturnsNullOpt)
{
    ObjectPtrField f;

    EXPECT_FALSE(f.Draw("L", nullptr, nullptr, "##p").has_value());
}
