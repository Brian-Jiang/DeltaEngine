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
    //DMaterial(std::shared_ptr<DShader> shader);
    //DMaterial(std::shared_ptr<DShader> shader, const CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC& blendDesc,
    //    const CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL& depthStencilState);
    ~DMaterial();

    void Initialize(DShader* shader);

    DFUNCTION()
    void SetShader(DShader* shader);

    void SetBlendState(const CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC& blendDesc);
    
    void SetDepthStencilState(const CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL& depthStencilState);

    DFUNCTION()
    void AddTexture(DTexture* texture);

    DFUNCTION()
    DTexture* GetTexture(int index) const;

    DFUNCTION()
    DELTAENGINE_API DShader* GetShader() const;

    DELTAENGINE_API CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC GetBlendState() const;
    DELTAENGINE_API CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL GetDepthStencilState() const;

private:
    DPROPERTY()
    DShader* m_shader;

    CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC m_blendDesc;
    
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL m_depthStencilState;

    DPROPERTY()
    std::vector<DTexture*> m_textures;
};

DELTA_ENGINE_NS_END