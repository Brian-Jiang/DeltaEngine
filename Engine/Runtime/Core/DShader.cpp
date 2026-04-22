#include "DShader.h"

#include "Assets/AssetDatabaseLocator.h"
#include "Assets/DPrimaryAsset.h"
#include "Assets/IAssetDatabase.h"
#include "Graphics/DXUtils.h"
#include "Graphics/ShaderCompile.h"
#include "IO/IOManager.h"

#include <cassert>
#include <cstring>

namespace
{
DeltaEngine::TBulkData SerializeShaderBlobs(
    const Microsoft::WRL::ComPtr<IDxcBlob>& vertexBlob,
    const Microsoft::WRL::ComPtr<IDxcBlob>& pixelBlob)
{
    assert(vertexBlob && pixelBlob);

    const uint64_t vertexSize = static_cast<uint64_t>(vertexBlob->GetBufferSize());
    const uint64_t pixelSize = static_cast<uint64_t>(pixelBlob->GetBufferSize());
    const uint64_t totalSize = sizeof(uint64_t) + vertexSize + sizeof(uint64_t) + pixelSize;

    auto* buffer = new uint8_t[totalSize];
    uint8_t* cursor = buffer;

    std::memcpy(cursor, &vertexSize, sizeof(uint64_t));
    cursor += sizeof(uint64_t);
    std::memcpy(cursor, vertexBlob->GetBufferPointer(), vertexSize);
    cursor += vertexSize;

    std::memcpy(cursor, &pixelSize, sizeof(uint64_t));
    cursor += sizeof(uint64_t);
    std::memcpy(cursor, pixelBlob->GetBufferPointer(), pixelSize);

    DeltaEngine::TBulkData bulk;
    bulk.Set(buffer, totalSize);
    delete[] buffer;
    return bulk;
}

bool DeserializeShaderBlobs(
    const DeltaEngine::TBulkData& bulk,
    Microsoft::WRL::ComPtr<IDxcBlob>& outVertexBlob,
    Microsoft::WRL::ComPtr<IDxcBlob>& outPixelBlob)
{
    if (!bulk.IsValid())
        return false;

    const uint8_t* cursor = bulk.m_data;

    auto readBlob = [&](Microsoft::WRL::ComPtr<IDxcBlob>& outBlob) -> bool
    {
        uint64_t size = 0;
        std::memcpy(&size, cursor, sizeof(uint64_t));
        cursor += sizeof(uint64_t);

        Microsoft::WRL::ComPtr<IDxcUtils> utils;
        if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils))))
            return false;

        Microsoft::WRL::ComPtr<IDxcBlobEncoding> blob;
        if (FAILED(utils->CreateBlobFromPinned(cursor, static_cast<uint32_t>(size), DXC_CP_ACP, &blob)))
            return false;

        outBlob = blob;
        cursor += size;
        return true;
    };

    return readBlob(outVertexBlob) && readBlob(outPixelBlob);
}

DeltaEngine::TBulkData SerializeInputLayout(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout)
{
    uint64_t totalSize = sizeof(uint32_t);
    for (const auto& element : inputLayout)
    {
        const uint32_t nameLength = static_cast<uint32_t>(std::strlen(element.SemanticName));
        totalSize += sizeof(uint32_t) + nameLength + sizeof(uint32_t) * 6;
    }

    auto* buffer = new uint8_t[totalSize];
    uint8_t* cursor = buffer;

    auto write = [&]<typename T>(const T& value)
    {
        std::memcpy(cursor, &value, sizeof(T));
        cursor += sizeof(T);
    };

    const uint32_t count = static_cast<uint32_t>(inputLayout.size());
    write(count);

    for (const auto& element : inputLayout)
    {
        const uint32_t nameLength = static_cast<uint32_t>(std::strlen(element.SemanticName));
        write(nameLength);
        std::memcpy(cursor, element.SemanticName, nameLength);
        cursor += nameLength;

        write(static_cast<uint32_t>(element.SemanticIndex));
        write(static_cast<uint32_t>(element.Format));
        write(static_cast<uint32_t>(element.InputSlot));
        write(static_cast<uint32_t>(element.AlignedByteOffset));
        write(static_cast<uint32_t>(element.InputSlotClass));
        write(static_cast<uint32_t>(element.InstanceDataStepRate));
    }

    DeltaEngine::TBulkData bulk;
    bulk.Set(buffer, totalSize);
    delete[] buffer;
    return bulk;
}

bool DeserializeInputLayout(
    const DeltaEngine::TBulkData& bulk,
    std::vector<D3D12_INPUT_ELEMENT_DESC>& outLayout,
    std::vector<std::string>& outSemanticNames)
{
    if (!bulk.IsValid())
        return false;

    const uint8_t* cursor = bulk.m_data;

    auto read = [&]<typename T>(T& value)
    {
        std::memcpy(&value, cursor, sizeof(T));
        cursor += sizeof(T);
    };

    uint32_t count = 0;
    read(count);

    outLayout.resize(count);
    outSemanticNames.resize(count);

    for (uint32_t index = 0; index < count; ++index)
    {
        uint32_t nameLength = 0;
        read(nameLength);

        outSemanticNames[index].assign(reinterpret_cast<const char*>(cursor), nameLength);
        cursor += nameLength;

        uint32_t semanticIndex = 0;
        uint32_t format = 0;
        uint32_t inputSlot = 0;
        uint32_t byteOffset = 0;
        uint32_t slotClass = 0;
        uint32_t stepRate = 0;
        read(semanticIndex);
        read(format);
        read(inputSlot);
        read(byteOffset);
        read(slotClass);
        read(stepRate);

        D3D12_INPUT_ELEMENT_DESC& desc = outLayout[index];
        desc.SemanticName = outSemanticNames[index].c_str();
        desc.SemanticIndex = semanticIndex;
        desc.Format = static_cast<DXGI_FORMAT>(format);
        desc.InputSlot = inputSlot;
        desc.AlignedByteOffset = byteOffset;
        desc.InputSlotClass = static_cast<D3D12_INPUT_CLASSIFICATION>(slotClass);
        desc.InstanceDataStepRate = stepRate;
    }

    return true;
}
}

using namespace DeltaEngine;
using namespace Microsoft::WRL;

DShader::DShader()
    : m_vertexShaderBlob(nullptr)
    , m_pixelShaderBlob(nullptr)
    , m_sourcePath(L"")
    , m_vertexShaderEntryPoint(L"")
    , m_pixelShaderEntryPoint(L"")
    , m_vertexShaderTargetProfile(L"")
    , m_pixelShaderTargetProfile(L"")
{
}

DShader::~DShader() = default;

void DShader::Initialize(
    const std::wstring& sourcePath,
    const std::wstring& vertexShaderEntryPoint,
    const std::wstring& pixelShaderEntryPoint,
    const std::wstring& vertexShaderTargetProfile,
    const std::wstring& pixelShaderTargetProfile)
{
    m_sourcePath = sourcePath;
    m_vertexShaderEntryPoint = vertexShaderEntryPoint;
    m_pixelShaderEntryPoint = pixelShaderEntryPoint;
    m_vertexShaderTargetProfile = vertexShaderTargetProfile;
    m_pixelShaderTargetProfile = pixelShaderTargetProfile;
    CompileShader();
}

void DShader::SetSourcePath(const std::wstring& sourcePath)
{
    m_sourcePath = sourcePath;
    CompileShader();
}

void DShader::SetVertexShaderEntryPoint(const std::wstring& entryPoint)
{
    m_vertexShaderEntryPoint = entryPoint;
    CompileShader();
}

void DShader::SetPixelShaderEntryPoint(const std::wstring& entryPoint)
{
    m_pixelShaderEntryPoint = entryPoint;
    CompileShader();
}

void DShader::SetVertexShaderTargetProfile(const std::wstring& targetProfile)
{
    m_vertexShaderTargetProfile = targetProfile;
    CompileShader();
}

void DShader::SetPixelShaderTargetProfile(const std::wstring& targetProfile)
{
    m_pixelShaderTargetProfile = targetProfile;
    CompileShader();
}

void DShader::Reimport()
{
    CompileShader();
    OnBeforeSerialize();
    if (DPrimaryAsset* asset = GetOwningAsset())
    {
        asset->MarkDirty();
        AssetDatabaseLocator::Get().SaveAsset(asset->GetAssetId());
    }
}

void DShader::SetInputLayout(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout)
{
    m_inputLayout.clear();
    m_inputLayoutSemanticNames.clear();
    m_inputLayout.reserve(inputLayout.size());
    m_inputLayoutSemanticNames.reserve(inputLayout.size());

    for (const auto& element : inputLayout)
    {
        m_inputLayoutSemanticNames.emplace_back(element.SemanticName ? element.SemanticName : "");
        D3D12_INPUT_ELEMENT_DESC ownedElement = element;
        ownedElement.SemanticName = m_inputLayoutSemanticNames.back().c_str();
        m_inputLayout.push_back(ownedElement);
    }
}

void DShader::CompileShader()
{
    m_vertexShaderBlob = CompileHLSLStage(m_sourcePath, m_vertexShaderEntryPoint, m_vertexShaderTargetProfile, "DShader VS");
    m_pixelShaderBlob = CompileHLSLStage(m_sourcePath, m_pixelShaderEntryPoint, m_pixelShaderTargetProfile, "DShader PS");
}

void DShader::OnBeforeSerialize()
{
    m_serializedShaderBlobs = SerializeShaderBlobs(m_vertexShaderBlob, m_pixelShaderBlob);
    m_serializedInputLayout = SerializeInputLayout(m_inputLayout);
}

void DShader::OnAfterDeserialize()
{
    DeserializeShaderBlobs(m_serializedShaderBlobs, m_vertexShaderBlob, m_pixelShaderBlob);
    DeserializeInputLayout(m_serializedInputLayout, m_inputLayout, m_inputLayoutSemanticNames);
}
