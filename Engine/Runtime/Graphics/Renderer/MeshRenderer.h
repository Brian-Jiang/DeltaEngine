#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/Renderer/Renderer.h"

#include "MeshRenderer.generated.h"

DELTA_ENGINE_NS_BEGIN

class MeshRenderProxy;
class DMesh;

DSTRUCT()
struct MeshRendererSettings
{
    DGENERATED_BODY_STRUCT(MeshRendererSettings)
};

DCLASS()
class DELTAENGINE_API MeshRenderer : public Renderer
{
    DGENERATED_BODY(MeshRenderer)

public:
    MeshRenderer();
    ~MeshRenderer();

    DFUNCTION()
    /// Sets the mesh rendered by this component.
    void SetMesh(DMesh* mesh);

protected:
    void InitGraphicState(std::shared_ptr<DXGraphicsContext> context) override;
    void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) override;
    void GatherShadowDrawCalls(std::shared_ptr<DXGraphicsContext> context, const ShadowView& view) override;
    void OnTransformChanged() override;
    std::shared_ptr<RenderProxy> DetachRenderProxyForRelease() override;

public:
    /// Rebuilds the mesh render proxy from the current mesh and settings.
    void CreateRenderProxy() override;

private:
    DPROPERTY()
    bool m_castShadow = true;

    DPROPERTY()
    DMesh* m_mesh;

    std::shared_ptr<MeshRenderProxy> m_meshRenderProxy;

    DPROPERTY()
    MeshRendererSettings m_settings;
};

DELTA_ENGINE_NS_END
