#include "Core/DMaterial.h"

using namespace DeltaEngine;

DMaterial::DMaterial()
    : m_shader(nullptr)
    , m_blendDesc()
    , m_depthStencilState()
{
}

DMaterial::DMaterial(std::shared_ptr<DShader> shader)
    : m_shader(shader)
    , m_blendDesc()
    , m_depthStencilState()
{
}

DMaterial::DMaterial(std::shared_ptr<DShader> shader, const CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC& blendDesc,
                     const CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL& depthStencilState)
    : m_shader(shader)
    , m_blendDesc(blendDesc)
    , m_depthStencilState(depthStencilState)
{
}

DMaterial::~DMaterial()
{
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
