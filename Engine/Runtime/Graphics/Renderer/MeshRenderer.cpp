#include "MeshRenderer.h"

#include "Runtime/Graphics/RenderProxy/MeshRenderProxy.h"

using namespace DeltaEngine;

MeshRenderer::MeshRenderer()
    : m_mesh(nullptr)
    , m_settings()
{
}

MeshRenderer::~MeshRenderer()
{
}

void MeshRenderer::SetMesh(DMesh* mesh)
{
    m_mesh = mesh;
    CreateRenderProxy();
}

void MeshRenderer::InitGraphicState(std::shared_ptr<DXGraphicsContext> context)
{
    if (!m_meshRenderProxy)
        return;

    m_meshRenderProxy->Initialize(context);
    m_meshRenderProxy->UpdateWorldTransform(GetWorldTransform());
}

void MeshRenderer::CreateRenderProxy()
{
    if (!m_mesh)
    {
        m_meshRenderProxy.reset();
        return;
    }

    m_meshRenderProxy = std::make_shared<MeshRenderProxy>(m_mesh, std::make_shared<MeshRendererSettings>(m_settings));
}

void DeltaEngine::MeshRenderer::GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context)
{
    if (m_meshRenderProxy)
        m_meshRenderProxy->GatherDrawCalls(context);
}

void MeshRenderer::GatherShadowDrawCalls(std::shared_ptr<DXGraphicsContext> context, const ShadowView& view)
{
    if (m_meshRenderProxy)
        m_meshRenderProxy->GatherShadowDrawCalls(context, view);
}

void DeltaEngine::MeshRenderer::OnTransformChanged()
{
    if (m_meshRenderProxy)
        m_meshRenderProxy->UpdateWorldTransform(GetWorldTransform());
}
