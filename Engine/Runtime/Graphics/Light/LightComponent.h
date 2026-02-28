#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Graphics/DXGraphicsContext.h"

#include "LightComponent.generated.h"

DELTA_ENGINE_NS_BEGIN

class DWorld;

DCLASS()
class LightComponent : public SceneComponent
{
    DGENERATED_BODY(LightComponent)
    friend class DWorld;

public:
    LightComponent() = default;
    LightComponent(std::string name) : SceneComponent(name) {}
    LightComponent(std::string name, std::shared_ptr<GameObject> gameObject) : SceneComponent(name, gameObject) {}

protected:
    virtual void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) = 0;
};

DELTA_ENGINE_NS_END