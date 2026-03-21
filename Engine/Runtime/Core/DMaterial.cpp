#include "Core/DMaterial.h"

using namespace DeltaEngine;

DMaterial::DMaterial()
    : m_shader(nullptr)
    , m_blendDesc()
    , m_depthStencilState()
{
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

void DMaterial::AddTexture(DTexture* texture)
{
    m_textures.push_back(texture);
}

DTexture* DMaterial::GetTexture(int index) const
{
    if (index < 0)
        return nullptr;

    const size_t textureIndex = static_cast<size_t>(index);
    return textureIndex < m_textures.size() ? m_textures[textureIndex] : nullptr;
}

DShader* DMaterial::GetShader() const { return m_shader; }

CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC DMaterial::GetBlendState() const { return m_blendDesc; }

CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DMaterial::GetDepthStencilState() const { return m_depthStencilState; }
