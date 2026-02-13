#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>
#include <memory>

DELTA_ENGINE_NS_BEGIN

class DXGraphicsContext;

class CameraRenderProxy
{

public:
    void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext);

private:
    DirectX::XMMATRIX m_viewMatrix;
    DirectX::XMMATRIX m_projectionMatrix;
};

DELTA_ENGINE_NS_END