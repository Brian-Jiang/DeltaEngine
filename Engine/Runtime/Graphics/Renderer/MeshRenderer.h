#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include "Graphics/Texture.h"
#include "Core/Component.h"
#include "Runtime/Graphics/Mesh.h"

DELTA_ENGINE_NS_BEGIN

class EngineMain;

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

    void Start(const std::vector<Mesh*> meshes, const std::vector<DirectX::XMMATRIX> meshTransforms, const Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap);
    void Render(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList, const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& srtHeap, const Microsoft::WRL::ComPtr<ID3D12Device>& device) const;

private:
    void LoadTexture(const Texture* texture, const Microsoft::WRL::ComPtr<ID3D12Device>& device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& srvHeap, UINT descriptorIndex);
	void AddMesh(const Mesh* mesh, const DirectX::XMMATRIX meshTransform, const Microsoft::WRL::ComPtr<ID3D12Device>& device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& srvHeap);

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> vertexBuffers;
    std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferViews;

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> indexBuffers;
    std::vector<D3D12_INDEX_BUFFER_VIEW> indexBufferViews;

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> transformCBs;
    //std::vector<D3D12_CONSTANT_BUFFER_VIEW> indexBufferViews;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_diffuseTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_normalTexture;

    //Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
    //Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    //Mesh mesh;

	std::vector<Mesh*> meshes;
    std::vector<DirectX::XMMATRIX> meshTransforms;

	int meshCount;

    static const UINT TexturePixelSize = 4;
};

DELTA_ENGINE_NS_END
