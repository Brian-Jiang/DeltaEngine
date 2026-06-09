#include "Runtime/Graphics/RenderGraph/RenderGraphBuilder.h"

DELTA_ENGINE_NS_BEGIN

void RenderGraphBuilder::BeginPass(size_t passIndex)
{
    if (passIndex >= m_passAccesses.size())
    {
        m_passAccesses.resize(passIndex + 1);
    }
    m_currentPass = passIndex;
}

void RenderGraphBuilder::Read(RenderGraphTextureHandle texture, D3D12_RESOURCE_STATES state)
{
    DELTA_ASSERT(texture.IsValid());
    DELTA_ASSERT(m_currentPass < m_passAccesses.size());
    m_passAccesses[m_currentPass].push_back({ texture, state, RenderGraphAccessType::Read });
}

void RenderGraphBuilder::Write(RenderGraphTextureHandle texture, D3D12_RESOURCE_STATES state)
{
    DELTA_ASSERT(texture.IsValid());
    DELTA_ASSERT(m_currentPass < m_passAccesses.size());
    m_passAccesses[m_currentPass].push_back({ texture, state, RenderGraphAccessType::Write });
}

DELTA_ENGINE_NS_END
