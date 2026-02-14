#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <DirectXMath.h>

DELTA_ENGINE_NS_BEGIN

struct DXGraphicsContext;

class SpotLightRenderProxy
{
public:
    void UpdateParameters(DirectX::XMVECTOR position, DirectX::XMVECTOR direction,
        DirectX::XMVECTOR color, float intensity, float range,
        float innerConeAngle, float outerConeAngle);
    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext);

private:
    DirectX::XMVECTOR m_position;
    DirectX::XMVECTOR m_direction;
    DirectX::XMVECTOR m_color;
    float m_intensity;
    float m_range;
    float m_innerConeAngle;
    float m_outerConeAngle;
};

DELTA_ENGINE_NS_END
