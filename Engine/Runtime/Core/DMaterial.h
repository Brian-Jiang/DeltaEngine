#pragma once

#include "EngineIncludes.h"

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
    DELTAENGINE_API DMaterial();
    DELTAENGINE_API ~DMaterial();

    /// Initializes the material with a shader.
    DELTAENGINE_API void Initialize(DShader* shader);

    /// Replaces the shader used by this material.
    DFUNCTION()
    DELTAENGINE_API void SetShader(DShader* shader);

    /// Sets the pipeline blend state used by this material.
    DELTAENGINE_API void SetBlendState(const CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC& blendDesc);
    
    /// Sets the pipeline depth-stencil state used by this material.
    DELTAENGINE_API void SetDepthStencilState(const CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL& depthStencilState);

    /// Appends a texture to the material texture list.
    DFUNCTION()
    DELTAENGINE_API void AddTexture(DTexture* texture);

    /// Returns the texture at the requested slot, or nullptr.
    DFUNCTION()
    DELTAENGINE_API DTexture* GetTexture(int index) const;

    /// Returns the shader currently assigned to the material.
    DFUNCTION()
    DELTAENGINE_API DShader* GetShader() const;

    /// Returns the current blend state descriptor.
    DELTAENGINE_API CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC GetBlendState() const;
    /// Returns the current depth-stencil state descriptor.
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
