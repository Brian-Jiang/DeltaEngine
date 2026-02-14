#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <DirectXMath.h>

DELTA_ENGINE_NS_BEGIN

struct DXGraphicsContext;

class DirectionalLightRenderProxy
{

public:
    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext);

private:
    DirectX::XMVECTOR m_direction;
    DirectX::XMVECTOR m_color;
    float m_intensity;
};

DELTA_ENGINE_NS_END
