#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <vector>
#include <DirectXMath.h>
#include <wrl.h>

#include "Graphics/Mesh.h"

DELTA_ENGINE_NS_BEGIN

class CommandList;

struct InstanceData {
    DirectX::XMFLOAT4X4 transform;
    DirectX::XMFLOAT3 color;
};

class InstancedDrawer {
public:
    InstancedDrawer(std::shared_ptr<Mesh> mesh, UINT32 count);
    void SetTransform(UINT32 index, DirectX::XMMATRIX transform);
    void SetColor(UINT32 index, DirectX::XMFLOAT3 color);
    void CreateBuffer(const Microsoft::WRL::ComPtr<ID3D12Device>& device);

    /// Draw using the CommandList wrapper.
    void Draw(const std::shared_ptr<CommandList>& commandList);

    Microsoft::WRL::ComPtr<ID3D12Resource> GetInstanceBuffer() { return m_instanceBuffer; }

private:
    std::shared_ptr<Mesh> m_mesh;
    std::vector<InstanceData> m_instanceData;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_instanceBuffer;
    UINT32 m_count;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    D3D12_VERTEX_BUFFER_VIEW m_instanceBufferView;
};

DELTA_ENGINE_NS_END
