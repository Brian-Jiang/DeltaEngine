#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <memory>

DELTA_ENGINE_NS_BEGIN

class Device;
class CommandList;

/// Passed to every renderer's InitGraphicState / GatherDrawCalls.
/// Extend this struct whenever renderers need access to a new engine resource.
struct DXGraphicsContext
{
    Device* device = nullptr;
    std::shared_ptr<CommandList> commandList;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap;
};

DELTA_ENGINE_NS_END
