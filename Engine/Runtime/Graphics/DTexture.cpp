#include "DTexture.h"

#include "IO/IOManager.h"
#include "Importers/TextureImporter.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"

using namespace DeltaEngine;

DTexture::DTexture(const std::string& filePath, bool isFullPath)
{
    m_name = filePath;
    std::string fullPath = filePath;
    if (!isFullPath)
        fullPath = IOManager::GetAssetFullPath(filePath);

    TextureImporter importer;
    importer.Import(fullPath);
    m_width = importer.width;
    m_height = importer.height;
    m_data = importer.data;
    m_pixelSize = importer.comp;
    m_format = DXGI_FORMAT_R8G8B8A8_UNORM;
}

std::shared_ptr<DTexture> DTexture::LoadFromFile(const std::string& filePath, bool isFullPath)
{
    return std::make_shared<DTexture>(filePath, isFullPath);
}

D3D12_CPU_DESCRIPTOR_HANDLE DTexture::GetSRVCPUHandle() const
{
    if (m_dx12Texture)
        return m_dx12Texture->GetShaderResourceView();
    return D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
}

ID3D12Resource* DTexture::GetD3D12Resource() const
{
    if (m_dx12Texture)
        return m_dx12Texture->GetD3D12Resource().Get();
    return nullptr;
}

void DTexture::SetGPUTexture(std::shared_ptr<DirectX12Texture> dx12Texture)
{
    m_dx12Texture = std::move(dx12Texture);
}
