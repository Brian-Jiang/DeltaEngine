#include "Graphics/Shadow/ShadowPassManager.h"

#include "Runtime/Core/DWorld.h"
#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/Shadow/ShadowView.h"

#include <pix3.h>
#include <unordered_map>

using namespace DeltaEngine;

void ShadowPassManager::Initialize(Device& device)
{
    Shutdown();
    m_directionalAtlas.Initialize(device, kAtlasSize, L"ShadowAtlas Directional");
    m_spotAtlas.Initialize(device, kAtlasSize, L"ShadowAtlas Spot");
    m_pointCubes.Initialize(device, kPointFaceSize, kPointCubeCount, L"ShadowCubeArray Point");
}

void ShadowPassManager::Shutdown()
{
    m_pointCubes.Shutdown();
    m_spotAtlas.Shutdown();
    m_directionalAtlas.Shutdown();
}

bool ShadowPassManager::ShadowResourcesReady() const
{
    return m_directionalAtlas.GetTexture() && m_spotAtlas.GetTexture() && m_pointCubes.GetTexture();
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
    if (!ShadowResourcesReady() || !ctx || !ctx->commandList)
        return;

    CommandList& commandList = *ctx->commandList;
    auto* d3dCL = commandList.GetD3D12CommandList().Get();
    PIXBeginEvent(d3dCL, PIX_COLOR_DEFAULT, L"ShadowPass");

    m_directionalAllocator.Reset(kAtlasSize, kAtlasSize, kDirectionalTileSize);
    m_spotAllocator.Reset(kAtlasSize, kAtlasSize, kSpotTileSize);
    m_pointAllocator.Reset(kPointCubeCount);

    std::vector<ShadowView> views;
    world.GatherShadowViews(ctx, views);

    std::unordered_map<uint32_t, int32_t> pointLightCube;

    for (const ShadowView& view : views)
    {
        ShadowMapTileRegion region {};
        switch (view.type)
        {
        case LightType::Directional:
            m_directionalAllocator.Allocate(kDefaultShadowMapEdge, region);
            break;
        case LightType::Spot:
            m_spotAllocator.Allocate(kDefaultShadowMapEdge, region);
            break;
        case LightType::Point:
            if (pointLightCube.contains(view.lightIndex))
                break;
            {
                const int32_t cubeIdx = m_pointAllocator.Allocate();
                if (cubeIdx >= 0)
                    pointLightCube[view.lightIndex] = cubeIdx;
            }
            break;
        }
    }

    commandList.ClearDepthStencilTexture(m_directionalAtlas.GetTexture(), D3D12_CLEAR_FLAG_DEPTH);
    commandList.ClearDepthStencilTexture(m_spotAtlas.GetTexture(), D3D12_CLEAR_FLAG_DEPTH);
    commandList.ClearDepthStencilTexture(m_pointCubes.GetTexture(), D3D12_CLEAR_FLAG_DEPTH);

    PIXEndEvent(d3dCL);
}
