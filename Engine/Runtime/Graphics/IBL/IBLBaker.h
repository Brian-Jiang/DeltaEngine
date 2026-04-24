#pragma once

#include "Runtime/EngineIncludes.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>

DELTA_ENGINE_NS_BEGIN

class Device;
class CommandQueue;
class RootSignature;
class PipelineStateObject;
class DirectX12Texture;

class IBLBaker
{
public:
    struct IBLResources
    {
        std::shared_ptr<DirectX12Texture> irradianceCube;
        std::shared_ptr<DirectX12Texture> specularCube;
        std::shared_ptr<DirectX12Texture> brdfLut;
        D3D12_CPU_DESCRIPTOR_HANDLE irradianceSRV{};
        D3D12_CPU_DESCRIPTOR_HANDLE specularSRV{};
        D3D12_CPU_DESCRIPTOR_HANDLE brdfLutSRV{};
    };

    DELTAENGINE_API IBLBaker();
    DELTAENGINE_API ~IBLBaker();

    DELTAENGINE_API void Initialize(Device& device);

    DELTAENGINE_API IBLResources Bake(Device& device,
                                     const std::shared_ptr<DirectX12Texture>& sourceCube);

    DELTAENGINE_API const IBLResources& GetStaticLut() const { return m_staticLut; }

    DELTAENGINE_API void Shutdown();

private:
    void CompilePipelines(Device& device);
    void BakeBrdfLut(Device& device);
    void BakeIrradiance(Device& device,
                        const std::shared_ptr<DirectX12Texture>& sourceCube,
                        IBLResources& out);
    void BakeSpecular(Device& device,
                      const std::shared_ptr<DirectX12Texture>& sourceCube,
                      IBLResources& out);

    std::shared_ptr<RootSignature>       m_iblRootSig;
    std::shared_ptr<PipelineStateObject> m_irradiancePSO;
    std::shared_ptr<RootSignature>       m_brdfLutRootSig;
    std::shared_ptr<PipelineStateObject> m_specularPSO;
    std::shared_ptr<PipelineStateObject> m_brdfLutPSO;

    IBLResources m_staticLut;

    bool m_initialized = false;
};

DELTA_ENGINE_NS_END
