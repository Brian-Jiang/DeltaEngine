#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/PostProcess/PostProcessPass.h"

#include <DirectXMath.h>
#include <memory>

#include "ColorGradingPass.generated.h"

DELTA_ENGINE_NS_BEGIN

class Device;
class RootSignature;
class PipelineStateObject;
struct DXGraphicsContext;

DCLASS()
class DELTAENGINE_API ColorGradingPass : public PostProcessPass
{
    DGENERATED_BODY(ColorGradingPass)

public:
    DPROPERTY()
    float m_saturation = 1.0f;

    DPROPERTY()
    float m_contrast = 1.0f;

    DPROPERTY()
    DirectX::XMFLOAT4 m_lift { 0.0f, 0.0f, 0.0f, 0.0f };

    DPROPERTY()
    DirectX::XMFLOAT4 m_gamma { 1.0f, 1.0f, 1.0f, 0.0f };

    DPROPERTY()
    DirectX::XMFLOAT4 m_gain { 1.0f, 1.0f, 1.0f, 0.0f };

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
