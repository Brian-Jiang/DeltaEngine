#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/RenderProxy/RenderProxy.h"

#include <memory>
#include <DirectXMath.h>

DELTA_ENGINE_NS_BEGIN

class DirectionalLightRenderProxy : public RenderProxy
{
public:
    void UpdateParameters(DirectX::XMVECTOR direction, DirectX::XMVECTOR color, float intensity);
    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext) override;

private:
    DirectX::XMVECTOR m_direction;
    DirectX::XMVECTOR m_color;
    float m_intensity;
};

DELTA_ENGINE_NS_END
