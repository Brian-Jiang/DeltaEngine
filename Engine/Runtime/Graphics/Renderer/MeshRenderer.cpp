#include "MeshRenderer.h"

#include "Runtime/Core/DMesh.h"
#include "Runtime/Graphics/RenderProxy/MeshRenderProxy.h"
#include "Runtime/Graphics/RenderProxy/RenderProxy.h"
#include "Runtime/Graphics/Shadow/ShadowView.h"
#include "Runtime/Reflection/DProperty.h"

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

void MeshRenderer::PostEditChangeProperty(const DProperty* prop)
{
    SceneComponent::PostEditChangeProperty(prop);

    if (prop && (prop->GetName() == "m_mesh" || prop->GetName() == "m_materialOverrides"))
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
        DLOG(LogRenderer, ELogLevel::Verbose, "MeshRenderer::CreateRenderProxy cleared (mesh is null)");
        return;
    }

    m_meshRenderProxy = std::make_shared<MeshRenderProxy>(m_mesh,
        std::make_shared<MeshRendererSettings>(m_settings), m_materialOverrides);
    m_meshRenderProxy->UpdateWorldTransform(GetWorldTransform());
    DLOG(LogRenderer, ELogLevel::Verbose, "MeshRenderer::CreateRenderProxy built proxy for mesh '{}'",
        m_mesh->GetSourcePath().stem().string());
}

void DeltaEngine::MeshRenderer::GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context)
{
    if (!m_mesh)
        return;
    if (!DELTA_ENSURE(m_meshRenderProxy))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "MeshRenderer::GatherDrawCalls: mesh set but render proxy is null; rebuilding");
        CreateRenderProxy();
        if (!m_meshRenderProxy)
            return;
    }
    m_meshRenderProxy->GatherDrawCalls(context);
}

void MeshRenderer::CollectTransparentDrawEntries(std::vector<TransparentDrawEntry>& out,
    DirectX::XMVECTOR cameraPosition) const
{
    if (!m_mesh || !m_meshRenderProxy)
        return;

    m_meshRenderProxy->AppendTransparentDrawEntries(out, cameraPosition);
}

void MeshRenderer::GatherShadowDrawCalls(std::shared_ptr<DXGraphicsContext> context, const ShadowView& view)
{
    if (!m_castShadow || !m_meshRenderProxy)
        return;
    m_meshRenderProxy->GatherShadowDrawCalls(context, view);
}

void DeltaEngine::MeshRenderer::OnTransformChanged()
{
    if (m_meshRenderProxy)
        m_meshRenderProxy->UpdateWorldTransform(GetWorldTransform());
}

std::shared_ptr<RenderProxy> MeshRenderer::DetachRenderProxyForRelease()
{
    return std::move(m_meshRenderProxy);
}
