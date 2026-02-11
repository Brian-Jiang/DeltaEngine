#include "DShader.h"

#include <iostream>
#include <dxcapi.h>

#include "IO/IOManager.h"
#include "Graphics/DXUtils.h"


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

DShader::DShader(const std::wstring& sourcePath, const std::wstring& vertexShaderEntryPoint,
                 const std::wstring& pixelShaderEntryPoint, const std::wstring& vertexShaderTargetProfile,
                 const std::wstring& pixelShaderTargetProfile)
    : m_vertexShaderBlob(nullptr)
    , m_pixelShaderBlob(nullptr)
    , m_sourcePath(sourcePath)
    , m_vertexShaderEntryPoint(vertexShaderEntryPoint)
    , m_pixelShaderEntryPoint(pixelShaderEntryPoint)
    , m_vertexShaderTargetProfile(vertexShaderTargetProfile)
    , m_pixelShaderTargetProfile(pixelShaderTargetProfile)
{
    CompileShader();
}

DeltaEngine::DShader::~DShader()
{
    //if (m_vertexShaderBlob)
    //{
    //    m_vertexShaderBlob->Release();
    //}

    //if (m_pixelShaderBlob)
    //{
    //    m_pixelShaderBlob->Release();
    //}
}

void DeltaEngine::DShader::SetSourcePath(const std::wstring& sourcePath)
{
    m_sourcePath = sourcePath;
    CompileShader();
}

void DeltaEngine::DShader::SetVertexShaderEntryPoint(const std::wstring& entryPoint)
{
    m_vertexShaderEntryPoint = entryPoint;
    CompileShader();
}

void DeltaEngine::DShader::SetPixelShaderEntryPoint(const std::wstring& entryPoint)
{
    m_pixelShaderEntryPoint = entryPoint;
    CompileShader();
}

void DeltaEngine::DShader::SetVertexShaderTargetProfile(const std::wstring& targetProfile)
{
    m_vertexShaderTargetProfile = targetProfile;
    CompileShader();
}

void DeltaEngine::DShader::SetPixelShaderTargetProfile(const std::wstring& targetProfile)
{
    m_pixelShaderTargetProfile = targetProfile;
    CompileShader();
}

void DeltaEngine::DShader::SetInputLayout(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout)
{
    m_inputLayout = inputLayout;
}

void DShader::CompileShader()
{
    ComPtr<IDxcUtils> dxcUtils;
    ComPtr<IDxcCompiler3> compiler;
    ComPtr<IDxcIncludeHandler> includeHandler;
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils)));
    ThrowIfFailed(dxcUtils->CreateDefaultIncludeHandler(&includeHandler));

    std::wstring shaderPath = IOManager::GetEngineSourceAssetFullPath(m_sourcePath);
    ComPtr<IDxcBlobEncoding> sourceBlob;
    ThrowIfFailed(dxcUtils->LoadFile(shaderPath.c_str(), nullptr, &sourceBlob));

    BOOL known;
    UINT32 encoding;
    ThrowIfFailed(sourceBlob->GetEncoding(&known, &encoding));
    DxcBuffer sourceBuffer { .Ptr = sourceBlob->GetBufferPointer(), .Size = sourceBlob->GetBufferSize(), .Encoding = encoding };

    // Vertex shader
    ComPtr<IDxcCompilerArgs> arguments;
    ThrowIfFailed(dxcUtils->BuildArguments(shaderPath.c_str(), m_vertexShaderEntryPoint.c_str(), m_vertexShaderTargetProfile.c_str(), nullptr, 0, nullptr, 0, &arguments));
    ComPtr<IDxcResult> vsResult;
    ThrowIfFailed(compiler->Compile(&sourceBuffer, arguments->GetArguments(), arguments->GetCount(), includeHandler.Get(), IID_PPV_ARGS(&vsResult)));
    HRESULT hr;
    ThrowIfFailed(vsResult->GetStatus(&hr));
    if (FAILED(hr))
    {
        ComPtr<IDxcBlobEncoding> error;
        vsResult->GetErrorBuffer(&error);
        std::string errorMessage(static_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize());
        std::cerr << errorMessage << std::endl;
    }

    vsResult->GetResult(&m_vertexShaderBlob);

    // Pixel shader
    ThrowIfFailed(dxcUtils->BuildArguments(shaderPath.c_str(), m_pixelShaderEntryPoint.c_str(), m_pixelShaderTargetProfile.c_str(), nullptr, 0, nullptr, 0, &arguments));
    ComPtr<IDxcResult> psResult;
    compiler->Compile(&sourceBuffer, arguments->GetArguments(), arguments->GetCount(), includeHandler.Get(), IID_PPV_ARGS(&psResult));
    psResult->GetStatus(&hr);
    if (FAILED(hr))
    {
        ComPtr<IDxcBlobEncoding> error;
        psResult->GetErrorBuffer(&error);
        std::string errorMessage(static_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize());
        std::cerr << errorMessage << std::endl;
    }

    psResult->GetResult(&m_pixelShaderBlob);
}
