#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/PostProcess/PostProcessPass.h"

#include <DirectXMath.h>
#include <memory>

#include "VignettePass.generated.h"

DELTA_ENGINE_NS_BEGIN

class Device;
class RootSignature;
class PipelineStateObject;
struct DXGraphicsContext;

DCLASS()
class DELTAENGINE_API VignettePass : public PostProcessPass
{
    DGENERATED_BODY(VignettePass)

public:
    DPROPERTY()
    float m_intensity = 0.4f;

    DPROPERTY()
    float m_smoothness = 0.5f;

    DPROPERTY()
    float m_roundness = 1.0f;

    DPROPERTY()
    DirectX::XMFLOAT4 m_color { 0.0f, 0.0f, 0.0f, 1.0f };

    void Initialize(Device& device) override;
    void Execute(DXGraphicsContext& ctx,
                 D3D12_CPU_DESCRIPTOR_HANDLE inputSRV,
                 D3D12_CPU_DESCRIPTOR_HANDLE outputRTV,
                 UINT width, UINT height) override;
    void Shutdown() override;

private:
    void LazyInitialize(DXGraphicsContext& ctx);

    std::shared_ptr<RootSignature> m_rootSignature;
    std::shared_ptr<PipelineStateObject> m_pso;
    bool m_initialized = false;
    bool m_loggedExecuteSkip = false;
};

DELTA_ENGINE_NS_END
