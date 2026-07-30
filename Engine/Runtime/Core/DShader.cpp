#include "Runtime/Core/DShader.h"

#include "Runtime/Assets/AssetDatabaseLocator.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Assets/IAssetDatabase.h"
#include "Runtime/Graphics/DXUtils.h"
#include "Runtime/Graphics/ShaderCompile.h"
#include "Runtime/IO/IOManager.h"
#include "Runtime/Logging/LogChannels.h"

#include <cstddef>
#include <cstring>
#include <filesystem>

using namespace DeltaEngine;

namespace
{
std::string PathLog(const std::filesystem::path& p)
{
    const std::u8string u = p.u8string();
    return { reinterpret_cast<const char*>(u.data()), u.size() };
}

bool BlobLooksValid(const Slang::ComPtr<ISlangBlob>& blob)
{
    return blob && blob->getBufferPointer() != nullptr && blob->getBufferSize() > 0;
}

DeltaEngine::TBulkData SerializeShaderBlobs(
    const Slang::ComPtr<ISlangBlob>& vertexBlob,
    const Slang::ComPtr<ISlangBlob>& pixelBlob)
{
    if (!BlobLooksValid(vertexBlob) || !BlobLooksValid(pixelBlob))
        return {};

    const uint64_t vertexSize = static_cast<uint64_t>(vertexBlob->getBufferSize());
    const uint64_t pixelSize = static_cast<uint64_t>(pixelBlob->getBufferSize());
    const uint64_t totalSize = sizeof(uint64_t) + vertexSize + sizeof(uint64_t) + pixelSize;

    auto* buffer = new uint8_t[totalSize];
    uint8_t* cursor = buffer;

    std::memcpy(cursor, &vertexSize, sizeof(uint64_t));
    cursor += sizeof(uint64_t);
    std::memcpy(cursor, vertexBlob->getBufferPointer(), static_cast<size_t>(vertexSize));
    cursor += static_cast<size_t>(vertexSize);

    std::memcpy(cursor, &pixelSize, sizeof(uint64_t));
    cursor += sizeof(uint64_t);
    std::memcpy(cursor, pixelBlob->getBufferPointer(), static_cast<size_t>(pixelSize));

    DeltaEngine::TBulkData bulk;
    bulk.Set(buffer, totalSize);
    delete[] buffer;
    return bulk;
}

bool DeserializeShaderBlobs(
    const DeltaEngine::TBulkData& bulk,
    Slang::ComPtr<ISlangBlob>& outVertexBlob,
    Slang::ComPtr<ISlangBlob>& outPixelBlob)
{
    outVertexBlob = nullptr;
    outPixelBlob = nullptr;

    if (!bulk.IsValid())
        return false;

    size_t off = 0;

    auto readBlob = [&](Slang::ComPtr<ISlangBlob>& outBlob, const char* label) -> bool {
        if (off + sizeof(uint64_t) > bulk.m_size)
        {
            DLOG(LogShader, ELogLevel::Warning,
                "DeserializeShaderBlobs: truncated before {} blob size prefix at offset {}",
                label,
                static_cast<unsigned long long>(off));
            return false;
        }

        uint64_t size = 0;
        std::memcpy(&size, bulk.m_data + off, sizeof(uint64_t));
        off += sizeof(uint64_t);

        if (off + size > bulk.m_size || size > static_cast<uint64_t>(SIZE_MAX))
        {
            DLOG(LogShader, ELogLevel::Warning,
                "DeserializeShaderBlobs: {} blob payload out of range declaredBytes={} bulkBytes={}",
                label,
                static_cast<unsigned long long>(size),
                static_cast<unsigned long long>(bulk.m_size));
            return false;
        }

        ISlangBlob* blob = slang_createBlob(bulk.m_data + off, static_cast<size_t>(size));
        if (!blob)
        {
            DLOG(LogShader, ELogLevel::Warning,
                "DeserializeShaderBlobs: slang_createBlob failed for {} size={}",
                label,
                static_cast<unsigned long long>(size));
            return false;
        }

        outBlob.attach(blob);
        off += static_cast<size_t>(size);
        return true;
    };

    return readBlob(outVertexBlob, "vertex") && readBlob(outPixelBlob, "pixel");
}

DeltaEngine::TBulkData SerializeInputLayout(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout)
{
    uint64_t totalSize = sizeof(uint32_t);
    for (const auto& element : inputLayout)
    {
        const uint32_t nameLength = static_cast<uint32_t>(std::strlen(element.SemanticName ? element.SemanticName : ""));
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
        const uint32_t nameLength = static_cast<uint32_t>(std::strlen(element.SemanticName ? element.SemanticName : ""));
        write(nameLength);
        if (nameLength > 0)
        {
            std::memcpy(cursor, element.SemanticName, nameLength);
            cursor += nameLength;
        }

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
    outLayout.clear();
    outSemanticNames.clear();

    if (!bulk.IsValid())
        return false;

    constexpr uint32_t kMaxElements = 128u;
    constexpr uint32_t kMaxSemanticLength = 512u;

    size_t off = 0;

    auto consume = [&](void* dst, size_t nbytes, const char* ctx) -> bool {
        if (off + nbytes > bulk.m_size)
        {
            DLOG(LogShader, ELogLevel::Warning,
                "DeserializeInputLayout: truncated ({}) offset {} need {} have {}",
                ctx,
                static_cast<unsigned long long>(off),
                static_cast<unsigned long long>(nbytes),
                static_cast<unsigned long long>(bulk.m_size));
            return false;
        }
        std::memcpy(dst, bulk.m_data + off, nbytes);
        off += nbytes;
        return true;
    };

    uint32_t count = 0;
    if (!consume(&count, sizeof(uint32_t), "element count"))
        return false;

    if (count > kMaxElements)
    {
        DLOG(LogShader, ELogLevel::Warning,
            "DeserializeInputLayout: unreasonable element count {} (max {})",
            count,
            kMaxElements);
        return false;
    }

    outLayout.resize(count);
    outSemanticNames.resize(count);

    for (uint32_t index = 0; index < count; ++index)
    {
        uint32_t nameLength = 0;
        if (!consume(&nameLength, sizeof(uint32_t), "semantic name length"))
            return false;

        if (nameLength > kMaxSemanticLength || off + nameLength > bulk.m_size)
        {
            DLOG(LogShader, ELogLevel::Warning,
                "DeserializeInputLayout: semantic name length {} invalid at offset {}",
                nameLength,
                static_cast<unsigned long long>(off));
            outLayout.clear();
            outSemanticNames.clear();
            return false;
        }

        outSemanticNames[index].assign(reinterpret_cast<const char*>(bulk.m_data + off), nameLength);
        off += nameLength;

        uint32_t semanticIndex = 0;
        uint32_t format = 0;
        uint32_t inputSlot = 0;
        uint32_t byteOffset = 0;
        uint32_t slotClass = 0;
        uint32_t stepRate = 0;
        if (!consume(&semanticIndex, sizeof(uint32_t), "semanticIndex"))
            return false;
        if (!consume(&format, sizeof(uint32_t), "format"))
            return false;
        if (!consume(&inputSlot, sizeof(uint32_t), "inputSlot"))
            return false;
        if (!consume(&byteOffset, sizeof(uint32_t), "byteOffset"))
            return false;
        if (!consume(&slotClass, sizeof(uint32_t), "slotClass"))
            return false;
        if (!consume(&stepRate, sizeof(uint32_t), "stepRate"))
            return false;

        D3D12_INPUT_ELEMENT_DESC& desc = outLayout[index];
        desc.SemanticName = outSemanticNames[index].c_str();
        desc.SemanticIndex = semanticIndex;
        desc.Format = static_cast<DXGI_FORMAT>(format);
        desc.InputSlot = inputSlot;
        desc.AlignedByteOffset = byteOffset;
        desc.InputSlotClass = static_cast<D3D12_INPUT_CLASSIFICATION>(slotClass);
        desc.InstanceDataStepRate = stepRate;
    }

    if (off != bulk.m_size)
        DLOG(LogShader, ELogLevel::Verbose,
            "DeserializeInputLayout: {} trailing bytes ignored after layout parse",
            static_cast<unsigned long long>(bulk.m_size - off));

    return true;
}
}


using namespace Microsoft::WRL;

DShader::DShader()
    : m_vertexShaderBlob(nullptr)
    , m_pixelShaderBlob(nullptr)
{
}

DShader::~DShader() = default;

void DShader::Initialize(
    const std::filesystem::path& sourcePath,
    const std::string& vertexShaderEntryPoint,
    const std::string& pixelShaderEntryPoint,
    const std::string& vertexShaderTargetProfile,
    const std::string& pixelShaderTargetProfile)
{
    m_sourcePath = sourcePath;
    m_vertexShaderEntryPoint = vertexShaderEntryPoint;
    m_pixelShaderEntryPoint = pixelShaderEntryPoint;
    m_vertexShaderTargetProfile = vertexShaderTargetProfile;
    m_pixelShaderTargetProfile = pixelShaderTargetProfile;
    CompileShader();
}

void DShader::SetSourcePath(const std::filesystem::path& sourcePath)
{
    m_sourcePath = sourcePath;
    CompileShader();
}

void DShader::SetVertexShaderEntryPoint(const std::string& entryPoint)
{
    m_vertexShaderEntryPoint = entryPoint;
    CompileShader();
}

void DShader::SetPixelShaderEntryPoint(const std::string& entryPoint)
{
    m_pixelShaderEntryPoint = entryPoint;
    CompileShader();
}

void DShader::SetVertexShaderTargetProfile(const std::string& targetProfile)
{
    m_vertexShaderTargetProfile = targetProfile;
    CompileShader();
}

void DShader::SetPixelShaderTargetProfile(const std::string& targetProfile)
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
    m_vertexShaderBlob = CompileSlangStage(m_sourcePath, m_vertexShaderEntryPoint, m_vertexShaderTargetProfile, "DShader VS");
    m_pixelShaderBlob = CompileSlangStage(m_sourcePath, m_pixelShaderEntryPoint, m_pixelShaderTargetProfile, "DShader PS");
    ++m_compileGeneration;

    if (!BlobLooksValid(m_vertexShaderBlob))
        DLOG(LogShader, ELogLevel::Error,
            "CompileShader: vertex stage empty path='{}' entry='{}' profile='{}'",
            PathLog(m_sourcePath),
            m_vertexShaderEntryPoint,
            m_vertexShaderTargetProfile);

    if (!BlobLooksValid(m_pixelShaderBlob))
        DLOG(LogShader, ELogLevel::Error,
            "CompileShader: pixel stage empty path='{}' entry='{}' profile='{}'",
            PathLog(m_sourcePath),
            m_pixelShaderEntryPoint,
            m_pixelShaderTargetProfile);
}

void DShader::OnBeforeSerialize()
{
    if (!BlobLooksValid(m_vertexShaderBlob) || !BlobLooksValid(m_pixelShaderBlob))
    {
        DLOG(LogShader, ELogLevel::Warning,
            "DShader::OnBeforeSerialize: missing VS/PS blobs for '{}' — omitting serialized bytecode",
            PathLog(m_sourcePath));
        m_serializedShaderBlobs = {};
    }
    else
        m_serializedShaderBlobs = SerializeShaderBlobs(m_vertexShaderBlob, m_pixelShaderBlob);

    m_serializedInputLayout = SerializeInputLayout(m_inputLayout);
}

void DShader::OnAfterDeserialize()
{
    if (!DeserializeShaderBlobs(m_serializedShaderBlobs, m_vertexShaderBlob, m_pixelShaderBlob))
    {
        DLOG(LogShader, ELogLevel::Warning,
            "DShader::OnAfterDeserialize: invalid serialized shader blobs for '{}' bulkBytes={}",
            PathLog(m_sourcePath),
            m_serializedShaderBlobs.m_size);
        m_vertexShaderBlob = nullptr;
        m_pixelShaderBlob = nullptr;
    }

    if (!DeserializeInputLayout(m_serializedInputLayout, m_inputLayout, m_inputLayoutSemanticNames))
    {
        DLOG(LogShader, ELogLevel::Warning,
            "DShader::OnAfterDeserialize: invalid serialized input layout for '{}' bulkBytes={}",
            PathLog(m_sourcePath),
            m_serializedInputLayout.m_size);
        m_inputLayout.clear();
        m_inputLayoutSemanticNames.clear();
    }
}
