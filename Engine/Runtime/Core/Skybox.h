#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Graphics/RenderResourceReleaseQueue.h"

#include <memory>
#include <optional>

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

    DELTAENGINE_API void BeginDestroy() override;
    DELTAENGINE_API bool IsReadyForFinishDestroy() override;

    /// Compiles the skybox shader, constructs the render proxy, and eagerly uploads the
    /// cubemap + builds the PSO so the GPU cubemap is ready before the first frame.
    /// Call once after m_cubemapTexture and m_material are both set.
    DELTAENGINE_API void Initialize(std::shared_ptr<DXGraphicsContext> context);

    /// Records the skybox draw call into the active command list.
    DELTAENGINE_API void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context);

    /// Returns the render proxy, which owns the GPU cubemap texture (may be null until Initialize runs).
    DELTAENGINE_API const std::shared_ptr<SkyboxRenderProxy>& GetRenderProxy() const { return m_renderProxy; }

    /** Cubemap texture to sample. Must be a DDS cubemap. */
    DPROPERTY()
    DTexture* m_cubemapTexture = nullptr;

    /** Material holding the compiled skybox shader. */
    DPROPERTY()
    DMaterial* m_material = nullptr;

private:
    std::shared_ptr<SkyboxRenderProxy> m_renderProxy;
    std::optional<RenderResourceReleaseToken> m_renderReleaseToken;
};

DELTA_ENGINE_NS_END
