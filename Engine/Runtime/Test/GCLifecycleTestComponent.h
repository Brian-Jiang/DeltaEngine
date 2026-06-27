#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/DComponent.h"

#include "GCLifecycleTestComponent.generated.h"

DELTA_ENGINE_NS_BEGIN

/// Reflected test component that records GC lifecycle calls and lets a test gate
/// FinishDestroy readiness, so the sweep/pending-destroy state machine can be
/// exercised deterministically. The state lives in exported statics defined in
/// the .cpp so the engine DLL and the test executable share a single instance.
DCLASS()
class GCLifecycleTestComponent : public DComponent
{
    DGENERATED_BODY(GCLifecycleTestComponent)

public:
    DPROPERTY()
    DComponent* m_ref = nullptr;

    DELTAENGINE_API void BeginDestroy() override;
    DELTAENGINE_API bool IsReadyForFinishDestroy() override;
    DELTAENGINE_API void FinishDestroy() override;

    DELTAENGINE_API void DelegatePing();

    DELTAENGINE_API static void Reset();

    DELTAENGINE_API static int  s_beginDestroyCount;
    DELTAENGINE_API static int  s_finishDestroyCount;
    DELTAENGINE_API static int  s_delegatePingCount;
    DELTAENGINE_API static bool s_readyForFinishDestroy;
};

DELTA_ENGINE_NS_END
