#include "Runtime/Graphics/Shadow/ShadowPassManager.h"

#include "Runtime/Core/DWorld.h"
#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/RenderProxy/DirectionalLightRenderProxy.h"
#include "Runtime/Graphics/RenderProxy/RenderProxy.h"
#include "Runtime/Graphics/Shadow/ShadowDepthPSO.h"
#include "Runtime/Graphics/Shadow/ShadowView.h"

#include <pix3.h>

#include <stdexcept>
#include <unordered_map>
#include <vector>

using namespace DeltaEngine;

void ShadowPassManager::Initialize(Device& device, const ShadowAtlasSettings& atlasConfig)
{
    Shutdown();
    m_atlasConfig = atlasConfig;
    m_skipRenderIssuesLogged = false;
    m_directionalAtlas.Initialize(device, m_atlasConfig.atlasSize, "ShadowAtlas Directional");
    m_spotAtlas.Initialize(device, m_atlasConfig.atlasSize, "ShadowAtlas Spot");
    m_pointCubes.Initialize(device, m_atlasConfig.pointFaceSize, m_atlasConfig.pointCubeCount, "ShadowCubeArray Point");
    try
    {
        m_shadowDepthPso = std::make_unique<ShadowDepthPSO>(device);
    }
    catch (const std::exception& ex)
    {
        m_shadowDepthPso.reset();
        DLOG(LogShadow, ELogLevel::Error,
            "ShadowPassManager::Initialize failed: ShadowDepthPSO construction threw (what='{}', expected successful shadow depth PSO build)",
            ex.what());
    }
    catch (...)
    {
        m_shadowDepthPso.reset();
        DLOG(LogShadow, ELogLevel::Error,
            "ShadowPassManager::Initialize failed: ShadowDepthPSO construction threw a non-std exception (expected successful shadow depth PSO build)");
    }

    if (!ShadowResourcesReady())
    {
        DLOG(LogShadow, ELogLevel::Error,
            "ShadowPassManager::Initialize completed with incomplete GPU resources (directionalTex={}, spotTex={}, pointTex={}, depthPso={}; expected all non-null)",
            static_cast<const void*>(m_directionalAtlas.GetTexture().get()),
            static_cast<const void*>(m_spotAtlas.GetTexture().get()),
            static_cast<const void*>(m_pointCubes.GetTexture().get()),
            static_cast<const void*>(m_shadowDepthPso.get()));
    }
    else
    {
        DLOG(LogShadow, ELogLevel::Log,
            "ShadowPassManager initialized (atlasSize={}, directionalTilePx={}, spotTilePx={}, pointFacePx={}, pointCubes={})",
            m_atlasConfig.atlasSize, m_atlasConfig.directionalTileSize, m_atlasConfig.spotTileSize,
            m_atlasConfig.pointFaceSize, m_atlasConfig.pointCubeCount);
    }
}

void ShadowPassManager::Shutdown()
{
    m_shadowDepthPso.reset();
    m_pointCubes.Shutdown();
    m_spotAtlas.Shutdown();
    m_directionalAtlas.Shutdown();
    m_skipRenderIssuesLogged = false;
    DLOG(LogShadow, ELogLevel::Log, "ShadowPassManager shut down");
}

bool ShadowPassManager::ShadowResourcesReady() const
{
    return m_directionalAtlas.GetTexture() && m_spotAtlas.GetTexture() && m_pointCubes.GetTexture() && m_shadowDepthPso;
}

std::shared_ptr<DirectX12Texture> ShadowPassManager::GetDirectionalAtlasTexture() const
{
    return m_directionalAtlas.GetTexture();
}

std::shared_ptr<DirectX12Texture> ShadowPassManager::GetSpotAtlasTexture() const
{
    return m_spotAtlas.GetTexture();
}

std::shared_ptr<DirectX12Texture> ShadowPassManager::GetPointCubeArrayTexture() const
{
    return m_pointCubes.GetTexture();
}

void ShadowPassManager::Render(std::shared_ptr<DXGraphicsContext> ctx, DWorld& world)
{
    if (!ctx)
    {
        if (!m_skipRenderIssuesLogged)
        {
            m_skipRenderIssuesLogged = true;
            DLOG(LogShadow, ELogLevel::Warning,
                "ShadowPassManager::Render skipped: null DXGraphicsContext (expected valid frame context)");
        }
        return;
    }

    if (!ctx->commandList)
    {
        if (!m_skipRenderIssuesLogged)
        {
            m_skipRenderIssuesLogged = true;
            DLOG(LogShadow, ELogLevel::Warning,
                "ShadowPassManager::Render skipped: null command list on context (expected recording command list)");
        }
        return;
    }

    if (!ShadowResourcesReady())
    {
        if (!m_skipRenderIssuesLogged)
        {
            m_skipRenderIssuesLogged = true;
            DLOG(LogShadow, ELogLevel::Warning,
                "ShadowPassManager::Render skipped: GPU shadow resources not ready (expected Initialize to succeed)");
        }
        return;
    }

    m_skipRenderIssuesLogged = false;

    CommandList& commandList = *ctx->commandList;
    auto* d3dCL = commandList.GetD3D12CommandList().Get();
    PIXBeginEvent(d3dCL, PIX_COLOR_DEFAULT, L"ShadowPass");

    m_directionalAllocator.Reset(m_atlasConfig.atlasSize, m_atlasConfig.atlasSize, m_atlasConfig.directionalTileSize);
    m_spotAllocator.Reset(m_atlasConfig.atlasSize, m_atlasConfig.atlasSize, m_atlasConfig.spotTileSize);
    m_pointAllocator.Reset(m_pointCubes.GetCubeCount());

    m_warnedDirectional.clear();
    m_warnedSpot.clear();
    m_warnedPoint.clear();

    ctx->shadowQualityScalar = m_settings.m_qualityScalar;

    std::vector<ShadowView> views;
    world.GatherShadowViews(ctx, views);

    struct DirJob
    {
        ShadowView view;
        ShadowMapTileRegion region;
    };
    struct SpotJob
    {
        ShadowView view;
        ShadowMapTileRegion region;
    };
    struct PointJob
    {
        ShadowView view;
        int32_t cubeIndex;
    };
    std::vector<DirJob> dirJobs;
    dirJobs.reserve(views.size());
    std::vector<SpotJob> spotJobs;
    spotJobs.reserve(views.size());
    std::vector<PointJob> pointJobs;
    pointJobs.reserve(views.size());

    std::unordered_map<uint32_t, int32_t> pointCubeForLight;

    for (const ShadowView& view : views)
    {
        if (view.type == LightType::Directional && view.shadowMapEdgePx > 0)
        {
            ShadowMapTileRegion region {};
            if (m_directionalAllocator.Allocate(view.shadowMapEdgePx, region) >= 0)
            {
                dirJobs.push_back({ view, region });
            }
            else if (m_warnedDirectional.insert(view.lightIndex).second)
            {
                DLOG(LogShadow, ELogLevel::Warning,
                    "Directional shadow allocation failed (lightIndex={}, edgePx={}, atlas {}x{})",
                    view.lightIndex, view.shadowMapEdgePx,
                    m_directionalAtlas.GetSize(), m_directionalAtlas.GetSize());
            }
            continue;
        }
        if (view.type == LightType::Spot)
        {
            const uint32_t spotEdgePx = view.shadowMapEdgePx > 0 ? view.shadowMapEdgePx : kDefaultShadowMapEdge;
            ShadowMapTileRegion region {};
            if (m_spotAllocator.Allocate(spotEdgePx, region) < 0)
            {
                if (m_warnedSpot.insert(view.lightIndex).second)
                {
                    DLOG(LogShadow, ELogLevel::Warning,
                        "Spot shadow allocation failed (lightIndex={}, edgePx={}, atlas {}x{})",
                        view.lightIndex, spotEdgePx,
                        m_spotAtlas.GetSize(), m_spotAtlas.GetSize());
                }
                continue;
            }
            spotJobs.push_back({ view, region });
            continue;
        }
        if (view.type == LightType::Point)
        {
            int32_t cubeIndex = -1;
            auto it = pointCubeForLight.find(view.lightIndex);
            if (it != pointCubeForLight.end())
            {
                cubeIndex = it->second;
            }
            else
            {
                cubeIndex = m_pointAllocator.Allocate();
                pointCubeForLight.emplace(view.lightIndex, cubeIndex);
            }
            if (cubeIndex < 0)
            {
                if (m_warnedPoint.insert(view.lightIndex).second)
                {
                    DLOG(LogShadow, ELogLevel::Warning,
                        "Point shadow allocation failed (lightIndex={}, cube slots={})",
                        view.lightIndex, m_atlasConfig.pointCubeCount);
                }
                continue;
            }
            pointJobs.push_back({ view, cubeIndex });
            continue;
        }
    }

    commandList.ClearDepthStencilTexture(m_directionalAtlas.GetTexture(), D3D12_CLEAR_FLAG_DEPTH);
    commandList.ClearDepthStencilTexture(m_spotAtlas.GetTexture(), D3D12_CLEAR_FLAG_DEPTH);
    commandList.ClearDepthStencilTexture(m_pointCubes.GetTexture(), D3D12_CLEAR_FLAG_DEPTH);

    if (!dirJobs.empty())
    {
        for (DirJob& job : dirJobs)
        {
            if (auto* proxy = dynamic_cast<DirectionalLightRenderProxy*>(job.view.shadowParamsWriter))
                proxy->FinishShadowViewProj(job.region.width, job.region.height, job.view);

            const ShadowMapTileRegion& r = job.region;
            D3D12_VIEWPORT viewport = {};
            viewport.TopLeftX = static_cast<float>(r.x);
            viewport.TopLeftY = static_cast<float>(r.y);
            viewport.Width = static_cast<float>(r.width);
            viewport.Height = static_cast<float>(r.height);
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;

            const D3D12_RECT scissor = { static_cast<LONG>(r.x), static_cast<LONG>(r.y),
                static_cast<LONG>(r.x + r.width), static_cast<LONG>(r.y + r.height) };

            commandList.SetViewport(viewport);
            commandList.SetScissorRect(scissor);
            commandList.SetDepthOnlyRenderTarget(m_directionalAtlas.GetTexture());
            commandList.SetGraphicsRootSignature(m_shadowDepthPso->GetRootSignature());
            commandList.SetPipelineState(m_shadowDepthPso->GetPSO2D());
            commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            world.GatherShadowDrawCalls(ctx, job.view);
        }
    }

    if (!spotJobs.empty())
    {
        for (const SpotJob& job : spotJobs)
        {
            const ShadowMapTileRegion& r = job.region;
            D3D12_VIEWPORT viewport = {};
            viewport.TopLeftX = static_cast<float>(r.x);
            viewport.TopLeftY = static_cast<float>(r.y);
            viewport.Width = static_cast<float>(r.width);
            viewport.Height = static_cast<float>(r.height);
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;

            const D3D12_RECT scissor = { static_cast<LONG>(r.x), static_cast<LONG>(r.y),
                static_cast<LONG>(r.x + r.width), static_cast<LONG>(r.y + r.height) };

            commandList.SetViewport(viewport);
            commandList.SetScissorRect(scissor);
            commandList.SetDepthOnlyRenderTarget(m_spotAtlas.GetTexture());
            commandList.SetGraphicsRootSignature(m_shadowDepthPso->GetRootSignature());
            commandList.SetPipelineState(m_shadowDepthPso->GetPSO2D());
            commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            world.GatherShadowDrawCalls(ctx, job.view);
        }
    }

    if (!pointJobs.empty())
    {
        D3D12_VIEWPORT viewport = {};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(m_atlasConfig.pointFaceSize);
        viewport.Height = static_cast<float>(m_atlasConfig.pointFaceSize);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        const D3D12_RECT scissor = { 0, 0,
            static_cast<LONG>(m_atlasConfig.pointFaceSize), static_cast<LONG>(m_atlasConfig.pointFaceSize) };

        for (const PointJob& job : pointJobs)
        {
            commandList.SetViewport(viewport);
            commandList.SetScissorRect(scissor);
            commandList.SetDepthOnlyRenderTarget(m_pointCubes.GetTexture(),
                m_pointCubes.GetDSVForFace(static_cast<uint32_t>(job.cubeIndex), job.view.cubeFace));
            commandList.SetGraphicsRootSignature(m_shadowDepthPso->GetRootSignature());
            commandList.SetPipelineState(m_shadowDepthPso->GetPSOCube());
            commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            world.GatherShadowDrawCalls(ctx, job.view);
        }
    }

    for (const DirJob& job : dirJobs)
    {
        ShadowAllocation alloc {};
        ShadowMapAllocator::FillAtlasUVRect(m_atlasConfig.atlasSize, m_atlasConfig.atlasSize, job.region, alloc);
        if (job.view.shadowParamsWriter)
            job.view.shadowParamsWriter->WriteShadowParams(ctx, alloc);
    }

    for (const SpotJob& job : spotJobs)
    {
        ShadowAllocation alloc {};
        ShadowMapAllocator::FillAtlasUVRect(m_atlasConfig.atlasSize, m_atlasConfig.atlasSize, job.region, alloc);
        if (job.view.shadowParamsWriter)
            job.view.shadowParamsWriter->WriteShadowParams(ctx, alloc);
    }

    for (const PointJob& job : pointJobs)
    {
        if (job.view.cubeFace != 0)
            continue;
        ShadowAllocation alloc {};
        alloc.cubeArrayIndex = job.cubeIndex;
        if (job.view.shadowParamsWriter)
            job.view.shadowParamsWriter->WriteShadowParams(ctx, alloc);
    }

    PIXEndEvent(d3dCL);
}
