#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <d3dx12.h>
#include <memory>

#include "Runtime/Graphics/RenderGraph/RenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphResourceHandle.h"

DELTA_ENGINE_NS_BEGIN

class DXRenderManager;

class GBufferAlbedoBlitGraphPass final : public RenderGraphPass
{
public:
    GBufferAlbedoBlitGraphPass(RenderGraphTextureHandle albedoInput,
        RenderGraphTextureHandle sceneColorOutput,
        D3D12_CPU_DESCRIPTOR_HANDLE albedoSrv,
        D3D12_CPU_DESCRIPTOR_HANDLE sceneColorRtv,
        CD3DX12_VIEWPORT viewport,
        D3D12_RECT scissorRect,
        DXRenderManager* renderManager);

    const char* GetName() const override;
    void Setup(RenderGraphBuilder& builder) override;
    void Execute(const RenderGraphContext& context) const override;

private:
    RenderGraphTextureHandle m_albedoInput;
    RenderGraphTextureHandle m_sceneColorOutput;
    D3D12_CPU_DESCRIPTOR_HANDLE m_albedoSrv{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_sceneColorRtv{};
    CD3DX12_VIEWPORT m_viewport{};
    D3D12_RECT m_scissorRect{};
    DXRenderManager* m_renderManager = nullptr;
};

DELTA_ENGINE_NS_END
