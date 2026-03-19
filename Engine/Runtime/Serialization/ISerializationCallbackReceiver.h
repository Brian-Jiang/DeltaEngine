#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

class ISerializationCallbackReceiver
{
public:
    virtual ~ISerializationCallbackReceiver() = default;

    // Called before serialization to prepare the object for serialization
    virtual void OnBeforeSerialize() { }

    // Called after deserialization to restore the object's state
    virtual void OnAfterDeserialize() { }
};

DELTA_ENGINE_NS_END
