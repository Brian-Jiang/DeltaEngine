#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <DirectXMath.h>

DELTA_ENGINE_NS_BEGIN

class Device;
class CommandList;
class RootSignature;

/// Passed to every renderer's InitGraphicState / GatherDrawCalls.
/// Camera data (view, projection, position) is constant for the frame;
/// each renderer uses this plus its own model matrix for per-draw MVP.
struct DXGraphicsContext
{
    std::shared_ptr<Device> device;
    std::shared_ptr<CommandList> commandList;
    //std::shared_ptr<RootSignature> rootSignature;
    //Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap;

    /// Camera constant buffer data (read-only for renderers).
    DirectX::XMFLOAT4X4 viewMatrix;
    DirectX::XMFLOAT4X4 projectionMatrix;
    DirectX::XMFLOAT4 cameraPosition;
};

DELTA_ENGINE_NS_END
