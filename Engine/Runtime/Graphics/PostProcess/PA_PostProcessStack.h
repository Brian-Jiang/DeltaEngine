#pragma once

#include "EngineIncludes.h"

#include "Runtime/Assets/DPrimaryAsset.h"

#include <string>

#include "PA_PostProcessStack.generated.h"

DELTA_ENGINE_NS_BEGIN

class PostProcessPass;
class PostProcessStack;

DCLASS()
class DELTAENGINE_API PA_PostProcessStack : public DPrimaryAsset
{
    DGENERATED_BODY(PA_PostProcessStack)

public:
    static PA_PostProcessStack* Create();

    /** Returns the embedded PostProcessStack, resolving it from loaded objects if not cached. Null if none present. */
    PostProcessStack* GetStack() const;

    PostProcessPass* AddPass(const std::string& className);
    bool RemovePass(const std::string& className);

private:
    mutable PostProcessStack* m_stack = nullptr;
};

DELTA_ENGINE_NS_END
