#include "SpotLight.h"

#include "Graphics/DXGraphicsContext.h"
#include "Graphics/RenderProxy/SpotLightRenderProxy.h"

using namespace DeltaEngine;

SpotLight::SpotLight()
    : m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_intensity(1.0f)
    , m_range(10.0f)
    , m_innerConeAngle(DirectX::XM_PIDIV4)   // 45 degrees
    , m_outerConeAngle(DirectX::XM_PIDIV2)   // 90 degrees
{
    m_renderProxy = std::make_shared<SpotLightRenderProxy>();
}

SpotLight::~SpotLight()
{
}

RenderProxy* SpotLight::GetRenderProxy()
{
    return m_renderProxy.get();
}

void SpotLight::UpdateParameters(DirectX::XMVECTOR color, float intensity, float range,
    float innerConeAngle, float outerConeAngle)
{
    if (!DELTA_ENSURE(intensity >= 0.0f))
    {
        DLOG(LogLight, ELogLevel::Warning,
            "SpotLight::UpdateParameters rejected: intensity={} (expected >= 0)", intensity);
        return;
    }
    if (!DELTA_ENSURE(range > 0.0f))
    {
        DLOG(LogLight, ELogLevel::Warning,
            "SpotLight::UpdateParameters rejected: range={} (expected > 0)", range);
        return;
    }
    if (!DELTA_ENSURE(outerConeAngle > 0.0f))
    {
        DLOG(LogLight, ELogLevel::Warning,
            "SpotLight::UpdateParameters rejected: outerConeAngle={} (expected > 0)", outerConeAngle);
        return;
    }
    if (!DELTA_ENSURE(innerConeAngle >= 0.0f && innerConeAngle <= outerConeAngle))
    {
        DLOG(LogLight, ELogLevel::Warning,
            "SpotLight::UpdateParameters rejected: innerConeAngle={} outerConeAngle={} (expected 0 <= inner <= outer)",
            innerConeAngle, outerConeAngle);
        return;
    }

    m_color = color;
    m_intensity = intensity;
    m_range = range;
    m_innerConeAngle = innerConeAngle;
    m_outerConeAngle = outerConeAngle;
}

void SpotLight::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context)
{
    if (!DELTA_ENSURE(context))
    {
        DLOG(LogLight, ELogLevel::Warning,
            "SpotLight::PreGatherDrawCalls skipped: context is null");
        return;
    }
    DELTA_ASSERT(m_renderProxy);

    auto pos = GetWorldPosition();
    DirectX::XMVECTOR position = DirectX::XMVectorSet(pos.x, pos.y, pos.z, 1.0f);
    DirectX::XMVECTOR direction = DirectX::XMVector4Normalize(DirectX::XMVectorSetW(GetForward(), 0.0f));

    m_renderProxy->UpdateParameters(position, direction, m_color, m_intensity, m_range,
        m_innerConeAngle, m_outerConeAngle, m_castShadow, m_shadowBias, m_pcssLightSize,
        m_shadowNormalBias, m_shadowSlopeBias);
    m_renderProxy->PreGatherDrawCalls(context);

    if (!DELTA_ENSURE(!context->spotLights.empty()))
    {
        DLOG(LogLight, ELogLevel::Warning,
            "SpotLight::PreGatherDrawCalls: render proxy did not append a spot light buffer entry");
        return;
    }
    m_renderProxy->SetSpotLightBufferIndex(static_cast<uint32_t>(context->spotLights.size() - 1));
}
