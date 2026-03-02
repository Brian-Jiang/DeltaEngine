#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <wrl/client.h>
#include <string>
#include <d3d12.h>
#include <d3dx12.h>
#include <vector>

#include "Runtime/Core/DObject.h"

#include "DMaterial.generated.h"

DELTA_ENGINE_NS_BEGIN

class DShader;
class DTexture;

DCLASS()
class DMaterial : public DObject
{
    DGENERATED_BODY(DMaterial)

public:
    DMaterial();
    DMaterial(std::shared_ptr<DShader> shader) = delete;

    DELTAENGINE_API void Initialize(std::shared_ptr<DShader> shader);

    DMaterial(std::shared_ptr<DShader> shader, const CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC& blendDesc,
        const CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL& depthStencilState) = delete;

    DELTAENGINE_API void Initialize(std::shared_ptr<DShader> shader, const CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC& blendDesc,
        const CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL& depthStencilState);

    ~DMaterial();

    DFUNCTION()
    void SetShader(std::shared_ptr<DShader> shader);

    void SetBlendState(const CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC& blendDesc);
    
    void SetDepthStencilState(const CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL& depthStencilState);

    DFUNCTION()
    void AddTexture(std::shared_ptr<DTexture> texture);

    DFUNCTION()
    std::shared_ptr<DTexture> GetTexture(int index) const;

    DFUNCTION()
    std::shared_ptr<DShader> GetShader() const;
    DFUNCTION()
    CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC GetBlendState() const;
    DFUNCTION()
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL GetDepthStencilState() const;

private:
    DPROPERTY()
    std::shared_ptr<DShader> m_shader;

    CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC m_blendDesc;
    
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL m_depthStencilState;

    DPROPERTY()
    std::vector<std::shared_ptr<DTexture>> m_textures;
};

DELTA_ENGINE_NS_END