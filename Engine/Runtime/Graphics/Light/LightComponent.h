#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Runtime/Core/SceneComponent.h"

#include "LightComponent.generated.h"

DELTA_ENGINE_NS_BEGIN

class DWorld;
struct DXGraphicsContext;

DCLASS()
class LightComponent : public SceneComponent
{
    DGENERATED_BODY(LightComponent)
    friend class DWorld;

public:
    LightComponent() = default;

protected:
    virtual void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) = 0;
};

DELTA_ENGINE_NS_END
