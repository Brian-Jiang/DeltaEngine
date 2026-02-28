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
    DMaterial(std::shared_ptr<DShader> shader);
    DMaterial(std::shared_ptr<DShader> shader, const CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC& blendDesc,
        const CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL& depthStencilState);
    ~DMaterial();

    DFUNCTION()
    void SetShader(std::shared_ptr<DShader> shader);
    DFUNCTION()
    void SetBlendState(const CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC& blendDesc);
    DFUNCTION()
    void SetDepthStencilState(const CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL& depthStencilState);
    DFUNCTION()
    void AddTexture(std::shared_ptr<DTexture> texture);
    DFUNCTION()
    std::shared_ptr<DTexture> GetTexture(int index) const;

    DFUNCTION()
    inline std::shared_ptr<DShader> GetShader() const { return m_shader; }
    DFUNCTION()
    inline CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC GetBlendState() const { return m_blendDesc; }
    DFUNCTION()
    inline CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL GetDepthStencilState() const { return m_depthStencilState; }

private:
    DPROPERTY()
    std::shared_ptr<DShader> m_shader;
    DPROPERTY()
    CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC m_blendDesc;
    DPROPERTY()
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL m_depthStencilState;
    DPROPERTY()
    std::vector<std::shared_ptr<DTexture>> m_textures;
};

DELTA_ENGINE_NS_END