#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <vector>
#include <DirectXMath.h>
#include <wrl.h>

#include "Graphics/Mesh.h"

DELTA_ENGINE_NS_BEGIN

struct InstanceData {
    DirectX::XMFLOAT4X4 transform;
};

class InstancedDrawer {
public:
    InstancedDrawer(std::shared_ptr<Mesh> mesh, UINT32 count);
    void SetTransform(UINT32 index, DirectX::XMMATRIX transform);
    void CreateBuffer(const Microsoft::WRL::ComPtr<ID3D12Device>& device);

    Microsoft::WRL::ComPtr<ID3D12Resource> GetInstanceBuffer() { return m_instanceBuffer; }

private:
    std::shared_ptr<Mesh> m_mesh;
    std::vector<InstanceData> m_instanceData;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_instanceBuffer;
    UINT32 m_count;
};

DELTA_ENGINE_NS_END