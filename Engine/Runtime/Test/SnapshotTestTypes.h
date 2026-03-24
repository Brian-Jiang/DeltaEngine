#pragma once

#include "EngineIncludes.h"

#include <string>

#include "Runtime/Core/DComponent.h"
#include "Runtime/Core/SceneComponent.h"

#include "SnapshotTestTypes.generated.h"

DELTA_ENGINE_NS_BEGIN

class DWorld;
class IAssetDatabase;

DCLASS()
class DSnapshotTestComponentA : public DComponent
{
    DGENERATED_BODY(DSnapshotTestComponentA)

public:
    DPROPERTY()
    float m_value = 42.0f;

    DPROPERTY()
    std::string m_tag = "CompA";

    DPROPERTY()
    DComponent* m_sibling = nullptr;
};

DCLASS()
class DSnapshotTestComponentB : public DComponent
{
    DGENERATED_BODY(DSnapshotTestComponentB)

public:
    DPROPERTY()
    int m_count = 7;

    DPROPERTY()
    DComponent* m_sibling = nullptr;
};

DCLASS()
class DSnapshotTestSceneComponent : public SceneComponent
{
    DGENERATED_BODY(DSnapshotTestSceneComponent)

public:
    DPROPERTY()
    float m_customData = 3.14f;
};

struct DELTAENGINE_API SnapshotTests
{
    static bool RunInternalPtrRoundTrip(DWorld* world);
    static bool RunSceneComponentTransformPreservation(DWorld* world);
    static bool RunAll(DWorld* world);
};

DELTA_ENGINE_NS_END
