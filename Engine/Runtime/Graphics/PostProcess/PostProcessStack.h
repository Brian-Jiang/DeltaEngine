#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/DObject.h"

#include <vector>

#include "PostProcessStack.generated.h"

DELTA_ENGINE_NS_BEGIN

class PostProcessPass;

DCLASS()
class DELTAENGINE_API PostProcessStack : public DObject
{
    DGENERATED_BODY(PostProcessStack)

public:
    DPROPERTY()
    std::vector<PostProcessPass*> m_passes;

    PostProcessPass* GetPass(int index) const;
    int GetPassCount() const;
};

DELTA_ENGINE_NS_END
