#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Core/DObject.h"

#include "Skybox.generated.h"

DELTA_ENGINE_NS_BEGIN

class DTexture;
class DMaterial;
class SkyboxRenderProxy;
struct DXGraphicsContext;

DCLASS()
class Skybox : public DObject
{
    DGENERATED_BODY(Skybox)

public:
    DELTAENGINE_API Skybox();

    /// Compiles the skybox shader and constructs the render proxy.
    /// Call once after m_cubemapTexture and m_material are both set.
    DELTAENGINE_API void Initialize();

    /// Records the skybox draw call into the active command list.
    DELTAENGINE_API void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context);

    /** Cubemap texture to sample. Must be a DDS cubemap. */
    DPROPERTY()
    DTexture* m_cubemapTexture = nullptr;

    /** Material holding the compiled skybox shader. */
    DPROPERTY()
    DMaterial* m_material = nullptr;

private:
    std::shared_ptr<SkyboxRenderProxy> m_renderProxy;
};

DELTA_ENGINE_NS_END
