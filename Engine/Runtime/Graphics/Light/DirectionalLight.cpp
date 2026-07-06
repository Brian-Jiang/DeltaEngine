#include "DirectionalLight.h"

#include "Graphics/DXGraphicsContext.h"
#include "Graphics/RenderProxy/DirectionalLightRenderProxy.h"

using namespace DeltaEngine;

DirectionalLight::DirectionalLight()
    : m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_intensity(1.0f)
{
    m_renderProxy = std::make_shared<DirectionalLightRenderProxy>();
}

DirectionalLight::~DirectionalLight()
{
}

RenderProxy* DirectionalLight::GetRenderProxy()
{
    return m_renderProxy.get();
}

void DirectionalLight::UpdateParameters(DirectX::XMVECTOR color, float intensity)
{
    if (!DELTA_ENSURE(intensity >= 0.0f))
    {
        DLOG(LogLight, ELogLevel::Warning,
            "DirectionalLight::UpdateParameters rejected: intensity={} (expected >= 0)", intensity);
        return;
    }

    m_color = color;
    m_intensity = intensity;
}

void DirectionalLight::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context)
{
    if (!DELTA_ENSURE(context))
    {
        DLOG(LogLight, ELogLevel::Warning,
            "DirectionalLight::PreGatherDrawCalls skipped: context is null");
        return;
    }
    DELTA_ASSERT(m_renderProxy);

    DirectX::XMVECTOR direction = DirectX::XMVector4Normalize(DirectX::XMVectorSetW(GetForward(), 0.0f));
    m_renderProxy->UpdateParameters(direction, m_color, m_intensity, m_castShadow, m_shadowBias, m_pcssLightSize,
        m_shadowMaxDistance, m_shadowOrthoPadding, m_shadowResolution,
        m_shadowNormalBias, m_shadowSlopeBias, m_shadowCasterDistance);
    m_renderProxy->PreGatherDrawCalls(context);

    if (!DELTA_ENSURE(!context->directionalLights.empty()))
    {
        DLOG(LogLight, ELogLevel::Warning,
            "DirectionalLight::PreGatherDrawCalls: render proxy did not append a directional light buffer entry");
        return;
    }
    m_renderProxy->SetDirectionalLightBufferIndex(static_cast<uint32_t>(context->directionalLights.size() - 1u));
}
