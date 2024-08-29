#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include "Graphics/Texture.h"
#include "Core/Component.h"
#include "Runtime/Graphics/Mesh.h"

DELTA_ENGINE_NS_BEGIN

class MeshRenderer: public Component
{
private:
    //struct Vertex {
    //    DirectX::XMFLOAT3 position;
    //    DirectX::XMFLOAT4 color;
    //    DirectX::XMFLOAT2 uv;
    //    DirectX::XMFLOAT3 normal;
    //};

public:
    MeshRenderer();
    ~MeshRenderer();

    void Start(const Mesh& mesh, const Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap);
    void Render(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList, const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& srtHeap, const Microsoft::WRL::ComPtr<ID3D12Device>& device) const;

private:
    void LoadTexture(const Texture* texture, const Microsoft::WRL::ComPtr<ID3D12Device>& device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& srvHeap, UINT descriptorIndex);

    Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_diffuseTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_normalTexture;

    //Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
    //Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Mesh mesh;

    static const UINT TexturePixelSize = 4;
};

DELTA_ENGINE_NS_END
