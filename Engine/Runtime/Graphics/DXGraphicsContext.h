#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <wrl/client.h>

DELTA_ENGINE_NS_BEGIN

struct DXGraphicsContext {
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
};

DELTA_ENGINE_NS_END