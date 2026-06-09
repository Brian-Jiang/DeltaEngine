#include "Runtime/Graphics/RenderGraph/SceneShadowReadGraphPass.h"

#include "Runtime/Graphics/RenderGraph/RenderGraphBuilder.h"

DELTA_ENGINE_NS_BEGIN

SceneShadowReadGraphPass::SceneShadowReadGraphPass(RenderGraphTextureHandle directionalAtlas,
    RenderGraphTextureHandle spotAtlas,
    RenderGraphTextureHandle pointCubeArray)
    : m_directionalAtlas(directionalAtlas)
    , m_spotAtlas(spotAtlas)
    , m_pointCubeArray(pointCubeArray)
{
}

const char* SceneShadowReadGraphPass::GetName() const
{
    return "SceneShadowRead";
}

void SceneShadowReadGraphPass::Setup(RenderGraphBuilder& builder)
{
    builder.Read(m_directionalAtlas, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    builder.Read(m_spotAtlas, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    builder.Read(m_pointCubeArray, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void SceneShadowReadGraphPass::Execute(const RenderGraphContext&) const
{
}

DELTA_ENGINE_NS_END
