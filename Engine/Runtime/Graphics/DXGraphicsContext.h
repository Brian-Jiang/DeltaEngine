#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <optional>
#include <vector>
#include <DirectXMath.h>

#include "Runtime/Graphics/Structures/Camera.h"
#include "Runtime/Graphics/Structures/Light.h"

DELTA_ENGINE_NS_BEGIN

class Device;
class CommandList;
class RootSignature;
class DXRenderManager;
class CameraRenderProxy;

/// Passed to every renderer's InitGraphicState / GatherDrawCalls.
/// Camera data (view, projection, position) is constant for the frame;
/// each renderer uses this plus its own model matrix for per-draw MVP.
struct DXGraphicsContext
{
    /// Render manager that owns the frame resources.
    std::shared_ptr<DXRenderManager> renderManager;

    /// Device used to allocate graphics resources.
    std::shared_ptr<Device> device;

    /// Command list receiving renderer draw calls for the frame.
    std::shared_ptr<CommandList> commandList;

    /// Directional lights gathered for the frame.
    std::vector<DirectionalLightBuffer> directionalLights;

    /// Point lights gathered for the frame.
    std::vector<PointLightBuffer> pointLights;

    /// Spot lights gathered for the frame.
    std::vector<SpotLightBuffer> spotLights;

    /// Uploads the gathered light buffers to the command list.
    void ApplyLightBuffersToCommandList();

    /// When set, overrides the scene camera matrices for this frame.
    /// Applied after PreGatherDrawCalls, before GatherDrawCalls.
    std::optional<CameraCB> cameraOverride;

    /// Non-owning pointer to the active camera's render proxy for the frame.
    CameraRenderProxy* camera = nullptr;
};

DELTA_ENGINE_NS_END
