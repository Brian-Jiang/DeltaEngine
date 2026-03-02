#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Graphics/DXGraphicsContext.h"

#include "Renderer.generated.h"

DELTA_ENGINE_NS_BEGIN

class DWorld;

DCLASS()
class Renderer : public SceneComponent
{
    DGENERATED_BODY(Renderer)
    friend class DWorld;

public:
    Renderer();
    Renderer(std::string name) = delete;

    DELTAENGINE_API void Initialize(std::string name);

    Renderer(std::string name, std::shared_ptr<GameObject> gameObject) = delete;

    DELTAENGINE_API void Initialize(std::string name, std::shared_ptr<GameObject> gameObject);

protected:
    virtual void InitGraphicState(std::shared_ptr<DXGraphicsContext> context) = 0;
    virtual void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) = 0;
};

DELTA_ENGINE_NS_END
