#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/RenderProxy/RenderProxy.h"

#include <memory>
#include <DirectXMath.h>

DELTA_ENGINE_NS_BEGIN

class PointLightRenderProxy : public RenderProxy
{
public:
    void UpdateParameters(DirectX::XMVECTOR position, DirectX::XMVECTOR color, float intensity, float range);
    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext) override;

private:
    DirectX::XMVECTOR m_position;
    DirectX::XMVECTOR m_color;
    float m_intensity;
    float m_range;
};

DELTA_ENGINE_NS_END
