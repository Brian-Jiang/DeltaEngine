#include "Runtime/Core/DMaterial.h"

using namespace DeltaEngine;

namespace
{
CD3DX12_BLEND_DESC MakeOpaqueBlendDesc()
{
    return CD3DX12_BLEND_DESC(D3D12_DEFAULT);
}

CD3DX12_BLEND_DESC MakeTransparentBlendDesc()
{
    CD3DX12_BLEND_DESC blend(D3D12_DEFAULT);
    blend.RenderTarget[0].BlendEnable = TRUE;
    blend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    return blend;
}

CD3DX12_DEPTH_STENCIL_DESC MakeOpaqueDepthStencilDesc()
{
    return CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
}

CD3DX12_DEPTH_STENCIL_DESC MakeTransparentDepthStencilDesc()
{
    CD3DX12_DEPTH_STENCIL_DESC depth(D3D12_DEFAULT);
    depth.DepthEnable = TRUE;
    depth.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    depth.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    return depth;
}
}

DMaterial::DMaterial()
    : m_shader(nullptr)
    , m_blendDesc()
    , m_depthStencilState()
{
    ApplyRenderModePipelineState();
}

DMaterial::~DMaterial() = default;

void DMaterial::Initialize(DShader* shader)
{
    m_shader = shader;
}

void DMaterial::SetShader(DShader* shader)
{
    m_shader = shader;
}

void DMaterial::SetBlendState(const CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC& blendDesc)
{
    m_blendDesc = blendDesc;
}

void DMaterial::SetDepthStencilState(const CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL& depthStencilState)
{
    m_depthStencilState = depthStencilState;
}

void DMaterial::SetRenderMode(uint32_t mode)
{
    m_renderMode = mode;
    ApplyRenderModePipelineState();
}

void DMaterial::ApplyRenderModePipelineState()
{
    switch (static_cast<ERenderMode>(m_renderMode))
    {
    case ERenderMode::Transparent:
        m_blendDesc = CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC(MakeTransparentBlendDesc());
        m_depthStencilState = CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL(MakeTransparentDepthStencilDesc());
        break;
    case ERenderMode::Opaque:
    case ERenderMode::Masked:
    default:
        m_blendDesc = CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC(MakeOpaqueBlendDesc());
        m_depthStencilState = CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL(MakeOpaqueDepthStencilDesc());
        break;
    }
}

void DMaterial::SetAlbedoTexture(DTexture* texture)            { m_albedoTexture = texture; }
void DMaterial::SetNormalTexture(DTexture* texture)            { m_normalTexture = texture; }
void DMaterial::SetMetallicRoughnessTexture(DTexture* texture) { m_metallicRoughnessTexture = texture; }
void DMaterial::SetOcclusionTexture(DTexture* texture)         { m_occlusionTexture = texture; }
void DMaterial::SetEmissiveMaskTexture(DTexture* texture)      { m_emissiveMaskTexture = texture; }
void DMaterial::SetAlphaMaskTexture(DTexture* texture)         { m_alphaMaskTexture = texture; }

DTexture* DMaterial::GetTexture(int slot) const
{
    switch (static_cast<MaterialTextureSlot>(slot))
    {
    case MaterialTextureSlot::Albedo:            return m_albedoTexture;
    case MaterialTextureSlot::Normal:            return m_normalTexture;
    case MaterialTextureSlot::MetallicRoughness: return m_metallicRoughnessTexture;
    case MaterialTextureSlot::AO:                return m_occlusionTexture;
    case MaterialTextureSlot::Emissive:          return m_emissiveMaskTexture;
    case MaterialTextureSlot::AlphaMask:         return m_alphaMaskTexture;
    default:                                     return nullptr;
    }
}

DShader* DMaterial::GetShader() const { return m_shader; }

CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC DMaterial::GetBlendState() const
{
    if (static_cast<ERenderMode>(m_renderMode) == ERenderMode::Transparent)
        return CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC(MakeTransparentBlendDesc());
    return m_blendDesc;
}

CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DMaterial::GetDepthStencilState() const
{
    if (static_cast<ERenderMode>(m_renderMode) == ERenderMode::Transparent)
        return CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL(MakeTransparentDepthStencilDesc());
    return m_depthStencilState;
}

MaterialFlags DMaterial::ComputeFlags() const
{
    MaterialFlags flags = MaterialFlags::None;
    if (m_albedoTexture)            flags |= MaterialFlags::HasAlbedoMap;
    if (m_normalTexture)            flags |= MaterialFlags::HasNormalMap;
    if (m_metallicRoughnessTexture) flags |= MaterialFlags::HasMetallicRoughnessMap;
    if (m_occlusionTexture)         flags |= MaterialFlags::HasOcclusionMap;
    if (m_emissiveMaskTexture)      flags |= MaterialFlags::HasEmissiveMap;
    if (m_alphaMaskTexture)         flags |= MaterialFlags::HasAlphaMask;
    if (m_doubleSided)              flags |= MaterialFlags::DoubleSided;
    if (static_cast<ERenderMode>(m_renderMode) == ERenderMode::Masked)
        flags |= MaterialFlags::AlphaTest;
    if (static_cast<ERenderMode>(m_renderMode) == ERenderMode::Transparent)
        flags |= MaterialFlags::AlphaBlend;
    else
    {
        const CD3DX12_BLEND_DESC& blend = m_blendDesc;
        if (blend.RenderTarget[0].BlendEnable)
            flags |= MaterialFlags::AlphaBlend;
    }
    return flags;
}

MaterialFlags DMaterial::GetFlags() const
{
    return ComputeFlags();
}

void DMaterial::FillMaterialCB(MaterialCB& outCB) const
{
    outCB.baseColor         = m_baseColor;
    outCB.metallic          = m_metallic;
    outCB.roughness         = m_roughness;
    outCB.emissiveIntensity = m_emissiveIntensity;
    outCB.alphaCutoff       = m_alphaCutoff;
    outCB.emissiveColor     = m_emissiveColor;
    outCB.flags             = static_cast<uint32_t>(ComputeFlags());
    outCB._pad[0]           = 0u;
    outCB._pad[1]           = 0u;
    outCB._pad[2]           = 0u;
}
