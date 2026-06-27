#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/PostProcess/PostProcessPass.h"

#include <d3d12.h>

#include <memory>

#include <slang-com-ptr.h>

#include "BloomPass.generated.h"

DELTA_ENGINE_NS_BEGIN

class Device;
class PipelineStateObject;
class RootSignature;
struct DXGraphicsContext;

DCLASS()
class DELTAENGINE_API BloomPass : public PostProcessPass
{
    DGENERATED_BODY(BloomPass)

public:
    DPROPERTY()
    float m_threshold = 1.0f;

    DPROPERTY()
    float m_softKnee = 0.5f;

    DPROPERTY()
    float m_intensity = 0.4f;

    DPROPERTY()
    int m_blurIterations = 5;

    void Initialize(Device& device) override;
    void Execute(DXGraphicsContext& ctx,
                 D3D12_CPU_DESCRIPTOR_HANDLE inputSRV,
                 D3D12_CPU_DESCRIPTOR_HANDLE outputRTV,
                 UINT width, UINT height) override;
    void Shutdown() override;

private:
    struct BloomParamsGPU
    {
        float threshold = 1.0f;
        float softKnee = 0.5f;
        float intensity = 0.4f;
        float texelSize = 0.0f;
    };

    void LazyInitialize(DXGraphicsContext& ctx);
    std::shared_ptr<PipelineStateObject> CreatePSO(Device& device, ISlangBlob* vsBlob, ISlangBlob* psBlob,
        const wchar_t* name) const;
    void DrawFullscreen(DXGraphicsContext& ctx, const std::shared_ptr<PipelineStateObject>& pso,
        D3D12_CPU_DESCRIPTOR_HANDLE outputRTV, UINT width, UINT height, const BloomParamsGPU& params,
        D3D12_CPU_DESCRIPTOR_HANDLE srv0, D3D12_CPU_DESCRIPTOR_HANDLE srv1 = {}) const;

    std::shared_ptr<RootSignature> m_rootSignature;
    std::shared_ptr<RootSignature> m_compositeRootSignature;
    std::shared_ptr<PipelineStateObject> m_extractPso;
    std::shared_ptr<PipelineStateObject> m_blurPso;
    std::shared_ptr<PipelineStateObject> m_compositePso;
    Slang::ComPtr<ISlangBlob> m_vertexShaderBlob;
    bool m_initialized = false;
    bool m_loggedExecuteSkip = false;
};

DELTA_ENGINE_NS_END
