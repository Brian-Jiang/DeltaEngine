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
    DPROPERTY()
    PostProcessStack* m_stack = nullptr;

    static PA_PostProcessStack* Create();

    PostProcessPass* AddPass(const std::string& className);
    bool RemovePass(const std::string& className);
};

DELTA_ENGINE_NS_END
