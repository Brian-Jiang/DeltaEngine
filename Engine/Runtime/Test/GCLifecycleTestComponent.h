#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/DComponent.h"

#include "GCLifecycleTestComponent.generated.h"

DELTA_ENGINE_NS_BEGIN

/// Reflected test component that records GC lifecycle calls and lets a test gate
/// FinishDestroy readiness, so the sweep/pending-destroy state machine can be
/// exercised deterministically.
DCLASS()
class GCLifecycleTestComponent : public DComponent
{
    DGENERATED_BODY(GCLifecycleTestComponent)

public:
    DPROPERTY()
    DComponent* m_ref = nullptr;

    void BeginDestroy() override { ++s_beginDestroyCount; }
    bool IsReadyForFinishDestroy() override { return s_readyForFinishDestroy; }
    void FinishDestroy() override { ++s_finishDestroyCount; }

    static void Reset()
    {
        s_beginDestroyCount     = 0;
        s_finishDestroyCount    = 0;
        s_readyForFinishDestroy = true;
    }

    static inline int  s_beginDestroyCount     = 0;
    static inline int  s_finishDestroyCount    = 0;
    static inline bool s_readyForFinishDestroy = true;
};

DELTA_ENGINE_NS_END
