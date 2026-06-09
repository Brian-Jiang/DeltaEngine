#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>

#include "Runtime/Graphics/RenderGraph/RenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphResourceHandle.h"

DELTA_ENGINE_NS_BEGIN

class PostProcessPass;

class DELTAENGINE_API PostProcessRenderGraphPass final : public RenderGraphPass
{
public:
    PostProcessRenderGraphPass(PostProcessPass* pass,
        RenderGraphTextureHandle input,
        RenderGraphTextureHandle output,
        D3D12_CPU_DESCRIPTOR_HANDLE inputSRV,
        D3D12_CPU_DESCRIPTOR_HANDLE outputRTV,
        UINT width,
        UINT height);

    const char* GetName() const override;
    void Setup(RenderGraphBuilder& builder) override;
    void Execute(const RenderGraphContext& context) const override;

private:
    PostProcessPass* m_pass = nullptr;
    RenderGraphTextureHandle m_input;
    RenderGraphTextureHandle m_output;
    D3D12_CPU_DESCRIPTOR_HANDLE m_inputSRV{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_outputRTV{};
    UINT m_width = 0;
    UINT m_height = 0;
};

DELTA_ENGINE_NS_END
