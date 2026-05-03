#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/PostProcess/PostProcessPass.h"

#include <memory>

#include "TonemapPass.generated.h"

DELTA_ENGINE_NS_BEGIN

class Device;
class RootSignature;
class PipelineStateObject;
struct DXGraphicsContext;

DCLASS()
class DELTAENGINE_API TonemapPass : public PostProcessPass
{
    DGENERATED_BODY(TonemapPass)

public:
    DPROPERTY()
    float m_exposure = 1.0f;

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
