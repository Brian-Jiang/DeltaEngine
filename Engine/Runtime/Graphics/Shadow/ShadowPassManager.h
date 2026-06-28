#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/Shadow/ShadowAtlas.h"
#include "Runtime/Graphics/Shadow/ShadowConstants.h"
#include "Runtime/Graphics/Shadow/ShadowCubeArray.h"
#include "Runtime/Graphics/Shadow/ShadowSettings.h"
#include "Runtime/Graphics/Shadow/ShadowDepthPSO.h"
#include "Runtime/Graphics/Shadow/ShadowMapAllocator.h"
#include "Runtime/Settings/ShadowAtlasSettings.h"

#include <memory>
#include <unordered_set>

DELTA_ENGINE_NS_BEGIN

class Device;
class DirectX12Texture;
class DWorld;
struct DXGraphicsContext;

class ShadowPassManager
{
public:
    DELTAENGINE_API void Initialize(Device& device, const ShadowAtlasSettings& atlasConfig = ShadowAtlasSettings{});
    DELTAENGINE_API void Shutdown();
    DELTAENGINE_API void Render(std::shared_ptr<DXGraphicsContext> ctx, DWorld& world);

    DELTAENGINE_API bool ShadowResourcesReady() const;
    DELTAENGINE_API std::shared_ptr<DirectX12Texture> GetDirectionalAtlasTexture() const;
    DELTAENGINE_API std::shared_ptr<DirectX12Texture> GetSpotAtlasTexture() const;
    DELTAENGINE_API std::shared_ptr<DirectX12Texture> GetPointCubeArrayTexture() const;

    DELTAENGINE_API const ShadowDepthPSO* GetShadowDepthPSO() const { return m_shadowDepthPso.get(); }

    DELTAENGINE_API const ShadowSettings& GetSettings() const { return m_settings; }
    DELTAENGINE_API ShadowSettings& GetSettings() { return m_settings; }

private:
    static constexpr uint32_t kDefaultShadowMapEdge = 1024;

    ShadowAtlasSettings m_atlasConfig;
    ShadowAtlas m_directionalAtlas;
    ShadowAtlas m_spotAtlas;
    ShadowCubeArray m_pointCubes;
    ShadowMapAllocator m_directionalAllocator;
    ShadowMapAllocator m_spotAllocator;
    PointSliceAllocator m_pointAllocator;
    std::unique_ptr<ShadowDepthPSO> m_shadowDepthPso;
    ShadowSettings m_settings;

    bool m_skipRenderIssuesLogged = false;

    std::unordered_set<uint32_t> m_warnedDirectional;
    std::unordered_set<uint32_t> m_warnedSpot;
    std::unordered_set<uint32_t> m_warnedPoint;
};

DELTA_ENGINE_NS_END
