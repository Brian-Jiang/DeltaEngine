#include "Core/DMaterial.h"

using namespace DeltaEngine;

DMaterial::DMaterial()
    : m_shader(nullptr)
    , m_blendDesc()
    , m_depthStencilState()
{
}

//DMaterial::DMaterial(std::shared_ptr<DShader> shader)
//    : m_shader(shader)
//    , m_blendDesc()
//    , m_depthStencilState()
//{
//}
//
//DMaterial::DMaterial(std::shared_ptr<DShader> shader, const CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC& blendDesc,
//                     const CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL& depthStencilState)
//    : m_shader(shader)
//    , m_blendDesc(blendDesc)
//    , m_depthStencilState(depthStencilState)
//{
//}

DMaterial::~DMaterial()
{
}

void DMaterial::Initialize(std::shared_ptr<DShader> shader)
{
    m_shader = shader;
}

void DMaterial::SetShader(std::shared_ptr<DShader> shader)
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

void DeltaEngine::DMaterial::AddTexture(std::shared_ptr<DTexture> texture)
{
    m_textures.push_back(texture);
}

std::shared_ptr<DTexture> DMaterial::GetTexture(int index) const
{
    if (index < 0 || index >= m_textures.size())
    {
        return nullptr;
    }

    return m_textures[index];
}

std::shared_ptr<DShader> DMaterial::GetShader() const { return m_shader; }

CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC DMaterial::GetBlendState() const { return m_blendDesc; }

CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DMaterial::GetDepthStencilState() const { return m_depthStencilState; }
