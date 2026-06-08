#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/RenderProxy/RenderProxy.h"

#include <memory>

DELTA_ENGINE_NS_BEGIN

class PipelineStateObject;
class DirectX12Texture;
class DTexture;
class DMaterial;

class SkyboxRenderProxy : public RenderProxy
{
public:
    DELTAENGINE_API SkyboxRenderProxy(DTexture* cubemapTexture, DMaterial* material);

    /// Eagerly uploads the cubemap and builds the PSO. Must be called once before GatherDrawCalls.
    DELTAENGINE_API void Initialize(std::shared_ptr<DXGraphicsContext> renderContext) override;

    /// Records the skybox draw call.
    DELTAENGINE_API void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext) override;

    /// Returns the GPU-resident cubemap texture once the proxy has been initialized.
    const std::shared_ptr<DirectX12Texture>& GetGpuCubemap() const { return m_gpuCubemap; }

    DELTAENGINE_API bool HasExclusiveGPUResources() const override;
    DELTAENGINE_API void ReleaseSharedReferences() override;

private:
    DTexture*  m_cubemapTexture;
    DMaterial* m_material;

    std::shared_ptr<DirectX12Texture>    m_gpuCubemap;
    std::shared_ptr<PipelineStateObject> m_pso;

    bool m_initialized = false;
};

DELTA_ENGINE_NS_END
