#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

class ISerializationCallbackReceiver
{
public:
    virtual ~ISerializationCallbackReceiver() = default;

    /// Called before serialization to prepare derived state.
    virtual void OnBeforeSerialize() { }

    /// Called after deserialization to restore derived state.
    virtual void OnAfterDeserialize() { }
};

DELTA_ENGINE_NS_END
