#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <memory>

DELTA_ENGINE_NS_BEGIN

class DXDescriptorHeapAllocator {


private:
    D3D12_DESCRIPTOR_HEAP_TYPE m_heapType;
};

DELTA_ENGINE_NS_END