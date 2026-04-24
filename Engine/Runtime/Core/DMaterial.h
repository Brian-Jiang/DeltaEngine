#pragma once

#include "EngineIncludes.h"

#include <cstdint>
#include <d3d12.h>
#include <d3dx12.h>
#include <DirectXMath.h>
#include <vector>

#include "Runtime/Core/DObject.h"
#include "Runtime/Graphics/MaterialConstants.h"

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

    DELTAENGINE_API MaterialFlags GetFlags() const { return static_cast<MaterialFlags>(m_flags); }
    DELTAENGINE_API void SetFlags(MaterialFlags flags) { m_flags = static_cast<uint32_t>(flags); }

    DELTAENGINE_API void FillMaterialCB(MaterialCB& outCB) const;

private:
    DPROPERTY()
    DShader* m_shader;

    CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC m_blendDesc;
    
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL m_depthStencilState;

    DPROPERTY()
    std::vector<DTexture*> m_textures;

    DPROPERTY()
    DirectX::XMFLOAT4 m_baseColor { 1.f, 1.f, 1.f, 1.f };

    DPROPERTY()
    float m_metallic { 0.f };

    DPROPERTY()
    float m_roughness { 0.5f };

    DPROPERTY()
    DirectX::XMFLOAT4 m_emissiveColor { 0.f, 0.f, 0.f, 0.f };

    DPROPERTY()
    float m_emissiveIntensity { 1.f };

    DPROPERTY()
    float m_alphaCutoff { 0.5f };

    DPROPERTY()
    uint32_t m_flags { 0u };
};

DELTA_ENGINE_NS_END
