#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 6326)
#endif

#include "Runtime/Core/Camera.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Core/Time.h"
#include "Runtime/Core/WorldContext.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{

float GetCameraFloat(Camera& camera, const char* propertyName)
{
    DClass* dclass = camera.GetClass();
    DProperty* property = dclass ? dclass->FindPropertyByName(propertyName) : nullptr;
    EXPECT_NE(property, nullptr);
    return property ? *static_cast<float*>(property->GetValue(&camera)) : 0.f;
}

}

TEST(CoreUtility, Camera_UpdateAspectRatio_InvalidNonPositive_KeepsPreviousAspect)
{
    Camera camera;

    camera.UpdateParameters(DirectX::XM_PIDIV4, 2.f, 0.1f, 100.f);

    camera.UpdateAspectRatio(-5.f);

    EXPECT_FLOAT_EQ(GetCameraFloat(camera, "m_aspectRatio"), 2.f);
}

TEST(CoreUtility, Time_TickTime_IncrementsFrameSinceStart)
{
    Time localTime;

    const UINT64 framesBefore = Time::frameSinceStart;

    localTime.TickTime();

    EXPECT_GT(Time::frameSinceStart, framesBefore);
}

TEST(CoreUtility, WorldContext_Default_IsEditorWithNullWorld)
{
    WorldContext context;

    EXPECT_EQ(context.type, WorldType::Editor);
    EXPECT_EQ(context.world, nullptr);
}

TEST(CoreUtility, DObject_MarkDirty_WithoutOwningAsset_DoesNotCrash)
{
    DObject object;

    object.MarkDirty();
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
