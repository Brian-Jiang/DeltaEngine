#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>

#include <cstdint>
#include <memory>

DELTA_ENGINE_NS_BEGIN

class Device;
class PipelineStateObject;
class RootSignature;

/** Root constants consumed by TAA.slang (b0). */
struct TemporalResolveConstants
{
    /** Projection jitter in UV space; added to the pixel UV to sample the current frame unjittered. */
    DirectX::XMFLOAT2 jitterUv{ 0.f, 0.f };
    DirectX::XMFLOAT2 texelSize{ 0.f, 0.f };
    /** Weight of the current frame in the exponential moving average. */
    float blendFactor = 0.1f;
    /** 0 outputs the current frame unchanged. */
    uint32_t historyValid = 0;
};

enum class TemporalResolveRootParameter : uint32_t
{
    Textures,
    Constants,
    Count,
};

/// Persistent root signature + PSO for the TAA resolve; graph passes are rebuilt every frame.
class TemporalResolvePipeline
{
public:
    /** Lazily compiles TAA.slang and builds the pipeline; a failure is not retried until Shutdown. */
    DELTAENGINE_API bool Ensure(Device& device);
    DELTAENGINE_API void Shutdown();

    const std::shared_ptr<RootSignature>& GetRootSignature() const { return m_rootSignature; }
    const std::shared_ptr<PipelineStateObject>& GetPipelineState() const { return m_pso; }

private:
    std::shared_ptr<RootSignature> m_rootSignature;
    std::shared_ptr<PipelineStateObject> m_pso;
    bool m_failed = false;
};

DELTA_ENGINE_NS_END
