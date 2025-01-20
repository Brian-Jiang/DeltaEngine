#include "Runtime/Graphics/DirectX/Buffer.h"

using namespace DeltaEngine;

Buffer::Buffer( Device& device, const D3D12_RESOURCE_DESC& resDesc )
: Resource( device, resDesc )
{}

Buffer::Buffer( Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource )
: Resource( device, resource )
{}
