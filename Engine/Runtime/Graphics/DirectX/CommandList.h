#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <wrl.h>

//#include "Runtime/Core/SceneComponent.h"
//#include "Runtime/Graphics/DXGraphicsContext.h"

DELTA_ENGINE_NS_BEGIN

class CommandList {

public:
    void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap);

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> GetGraphicsCommandList() const { return m_CommandList; }
    
    
private:
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> m_CommandList;

    // Keep track of the currently bound descriptor heaps. Only change descriptor 
    // heaps if they are different than the currently bound descriptor heaps.
    ID3D12DescriptorHeap* m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
};

DELTA_ENGINE_NS_END