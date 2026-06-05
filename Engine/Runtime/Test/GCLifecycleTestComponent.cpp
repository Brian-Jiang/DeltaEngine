#include "Runtime/Test/GCLifecycleTestComponent.h"

using namespace DeltaEngine;

int  GCLifecycleTestComponent::s_beginDestroyCount     = 0;
int  GCLifecycleTestComponent::s_finishDestroyCount    = 0;
bool GCLifecycleTestComponent::s_readyForFinishDestroy = true;

void GCLifecycleTestComponent::BeginDestroy()
{
    ++s_beginDestroyCount;
}

bool GCLifecycleTestComponent::IsReadyForFinishDestroy()
{
    return s_readyForFinishDestroy;
}

void GCLifecycleTestComponent::FinishDestroy()
{
    ++s_finishDestroyCount;
}

void GCLifecycleTestComponent::Reset()
{
    s_beginDestroyCount     = 0;
    s_finishDestroyCount    = 0;
    s_readyForFinishDestroy = true;
}
