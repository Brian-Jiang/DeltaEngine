#include "Graphics/DirectX/ByteAddressBuffer.h"
#include "Graphics/DirectX/Device.h"


using namespace DeltaEngine;
using namespace Microsoft::WRL;

ByteAddressBuffer::ByteAddressBuffer( Device& device, const D3D12_RESOURCE_DESC& resDesc )
: Buffer( device, resDesc )
{}

ByteAddressBuffer::ByteAddressBuffer( Device& device, ComPtr<ID3D12Resource> resource )
: Buffer( device, resource )
{}

