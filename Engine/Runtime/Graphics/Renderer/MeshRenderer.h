#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include "Graphics/Texture.h"
#include "Core/Component.h"

namespace DeltaEngine
{

class MeshRenderer: public Component
{
private:
    struct Vertex {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
        DirectX::XMFLOAT2 uv;
        DirectX::XMFLOAT3 normal;
    };

public:
    MeshRenderer();
    ~MeshRenderer();

    void Start(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const char* diffuseTexturePath, const char* normalTexturePath, const Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap);
    void Render(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList) const;

private:
    void LoadTexture(const char* texturePath, const Microsoft::WRL::ComPtr<ID3D12Device>& device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& srvHeap, UINT descriptorIndex);

    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_diffuseTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_normalTexture;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;

    static const UINT TexturePixelSize = 4;
};

}
