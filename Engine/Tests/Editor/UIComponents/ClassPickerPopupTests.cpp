#include "Editor/EditorCoreFixture.h"

#include "Editor/UIComponents/ClassPickerPopup.h"

#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

#include <gtest/gtest.h>

#include <vector>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class UIComponentsEditorCoreFixture : public EditorCoreFixture
{
};

TEST_F(UIComponentsEditorCoreFixture, ClassPickerPopup_Open_WithNullClassEntry_DoesNotCrash)
{
    DClass* go = GetReflectionRegistry().FindClassByName("GameObject");
    ASSERT_NE(go, nullptr);
    ClassPickerPopup             p;
    std::vector<const DClass*> v = {nullptr, go};
    p.Open(std::move(v));
}

TEST_F(UIComponentsEditorCoreFixture, ClassPickerPopup_Open_EmptyList_DoesNotCrash)
{
    ClassPickerPopup p;
    p.Open({});
}
