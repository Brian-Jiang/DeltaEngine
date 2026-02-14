#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <DirectXMath.h>

#include "Runtime/Graphics/Structures/Light.h"

DELTA_ENGINE_NS_BEGIN

class Device;
class CommandList;
class RootSignature;
class DXRenderManager;

/// Passed to every renderer's InitGraphicState / GatherDrawCalls.
/// Camera data (view, projection, position) is constant for the frame;
/// each renderer uses this plus its own model matrix for per-draw MVP.
struct DXGraphicsContext
{
    std::shared_ptr<DXRenderManager> renderManager;
    std::shared_ptr<Device> device;
    std::shared_ptr<CommandList> commandList;

    std::vector<DirectionalLightBuffer> directionalLights;
    std::vector<PointLightBuffer> pointLights;
    std::vector<SpotLightBuffer> spotLights;

    void ApplyLightBuffersToCommandList();
};

DELTA_ENGINE_NS_END
