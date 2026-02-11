#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <wrl/client.h>
#include <string>
#include <d3d12.h>
#include <d3dx12.h>
#include <vector>

#include "Runtime/Core/DObject.h"

DELTA_ENGINE_NS_BEGIN

class DShader;
class DTexture;

class DMaterial : public DObject
{

public:
    DMaterial();
    DMaterial(std::shared_ptr<DShader> shader);
    DMaterial(std::shared_ptr<DShader> shader, const CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC& blendDesc,
        const CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL& depthStencilState);
    ~DMaterial();

    void SetShader(std::shared_ptr<DShader> shader);
    void SetBlendState(const CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC& blendDesc);
    void SetDepthStencilState(const CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL& depthStencilState);
    void AddTexture(std::shared_ptr<DTexture> texture);

    inline std::shared_ptr<DShader> GetShader() const { return m_shader; }
    inline CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC GetBlendState() const { return m_blendDesc; }
    inline CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL GetDepthStencilState() const { return m_depthStencilState; }

private:
    std::shared_ptr<DShader> m_shader;
    CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC m_blendDesc;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL m_depthStencilState;
    std::vector<std::shared_ptr<DTexture>> m_textures;
};

DELTA_ENGINE_NS_END