#pragma once

#include "EngineIncludes.h"

#include <memory>

DELTA_ENGINE_NS_BEGIN

class PipelineStateObject;
class DirectX12Texture;
struct DXGraphicsContext;
class DTexture;
class DMaterial;

class SkyboxRenderProxy
{
public:
    SkyboxRenderProxy(DTexture* cubemapTexture, DMaterial* material);

    /// Eagerly uploads the cubemap and builds the PSO. Must be called once before GatherDrawCalls.
    void Initialize(std::shared_ptr<DXGraphicsContext> renderContext);

    /// Records the skybox draw call.
    void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext);

    /// Returns the GPU-resident cubemap texture once the proxy has been initialized.
    const std::shared_ptr<DirectX12Texture>& GetGpuCubemap() const { return m_gpuCubemap; }

private:
    void BuildPipelineStateObject(std::shared_ptr<DXGraphicsContext> renderContext);

private:
    DTexture*  m_cubemapTexture;
    DMaterial* m_material;

    std::shared_ptr<DirectX12Texture>    m_gpuCubemap;
    std::shared_ptr<PipelineStateObject> m_pso;

    bool m_initialized = false;
};

DELTA_ENGINE_NS_END
