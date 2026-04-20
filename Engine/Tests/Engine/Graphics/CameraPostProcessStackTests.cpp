#include "Runtime/Core/Camera.h"
#include "Runtime/Core/UUID.h"
#include "Runtime/Graphics/PostProcess/PostProcessStack.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Serialization/ObjectSnapshot.h"
#include "Runtime/Serialization/ObjectSnapshotReader.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace DeltaEngine;

TEST(CameraPostProcessStackTests, CameraHasReflectedPostProcessStackPtrProperty)
{
    DClass* cls = GetReflectionRegistry().FindClassByName("Camera");
    ASSERT_NE(cls, nullptr);

    DProperty* prop = cls->FindPropertyByName("m_postProcessStack");
    ASSERT_NE(prop, nullptr);

    auto* ptrProp = dynamic_cast<DObjectPtrPropertyBase*>(prop);
    ASSERT_NE(ptrProp, nullptr);
    EXPECT_EQ(prop->GetPropertyType(), EPropertyType::ObjectPtr);
}

TEST(CameraPostProcessStackTests, SnapshotRoundTripPreservesStackReference)
{
    const ObjectId cameraId = ObjectId::Generate();
    const ObjectId stackId  = ObjectId::Generate();
    const std::string nullAssetId = DeltaEngine::UUID::Null().ToString();

    ObjectSnapshot snapshot;
    snapshot.rootClassName = "Camera";
    snapshot.rootJson = {
        {"objects", nlohmann::json::array({
            nlohmann::json{
                {"_class",    "Camera"},
                {"_objectId", cameraId.ToString()},
                {"m_postProcessStack", nlohmann::json{
                    {"assetId",  nullAssetId},
                    {"objectId", stackId.ToString()}
                }}
            },
            nlohmann::json{
                {"_class",    "PostProcessStack"},
                {"_objectId", stackId.ToString()}
            }
        })}
    };
    snapshot.capturedIds = { cameraId, stackId };

    ObjectSnapshotReader reader;
    DObject* restored = reader.Restore(snapshot, nullptr, nullptr);

    ASSERT_NE(restored, nullptr);
    auto* restoredCamera = dynamic_cast<Camera*>(restored);
    ASSERT_NE(restoredCamera, nullptr);

    DClass* cls = GetReflectionRegistry().FindClassByName("Camera");
    ASSERT_NE(cls, nullptr);
    DProperty* prop = cls->FindPropertyByName("m_postProcessStack");
    ASSERT_NE(prop, nullptr);

    DObject* resolved = prop->GetObjectPointer(restoredCamera);
    ASSERT_NE(resolved, nullptr);

    auto* restoredStack = dynamic_cast<PostProcessStack*>(resolved);
    ASSERT_NE(restoredStack, nullptr);
    EXPECT_EQ(restoredStack->GetObjectId(), stackId);
}
