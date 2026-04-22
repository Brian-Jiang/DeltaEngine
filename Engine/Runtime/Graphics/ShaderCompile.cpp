#include "Graphics/ShaderCompile.h"

#include "Graphics/DXUtils.h"
#include "IO/IOManager.h"

#include <cstdio>
#include <iostream>
#include <string>

using namespace Microsoft::WRL;

DELTA_ENGINE_NS_BEGIN

namespace
{
void WriteShaderPdb(IDxcResult* result)
{
    if (!result)
        return;

    ComPtr<IDxcBlob> pdb;
    ComPtr<IDxcBlobUtf16> pdbName;
    result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pdb), &pdbName);
    if (!pdb || !pdbName || !pdbName->GetStringPointer())
        return;

    FILE* file = nullptr;
    if (_wfopen_s(&file, pdbName->GetStringPointer(), L"wb") != 0 || file == nullptr)
        return;

    std::fwrite(pdb->GetBufferPointer(), pdb->GetBufferSize(), 1, file);
    std::fclose(file);
}
}

ComPtr<IDxcBlob> CompileHLSLStage(
    const std::wstring& engineRelativePath,
    const std::wstring& entryPoint,
    const std::wstring& targetProfile,
    const char* debugLabel)
{
    ComPtr<IDxcUtils> dxcUtils;
    ComPtr<IDxcCompiler3> compiler;
    ComPtr<IDxcIncludeHandler> includeHandler;
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils)));
    ThrowIfFailed(dxcUtils->CreateDefaultIncludeHandler(&includeHandler));

    const std::wstring shaderPath = IOManager::GetEngineSourceAssetFullPath(engineRelativePath);
    ComPtr<IDxcBlobEncoding> sourceBlob;
    ThrowIfFailed(dxcUtils->LoadFile(shaderPath.c_str(), nullptr, &sourceBlob));

    BOOL known = FALSE;
    UINT32 encoding = 0;
    ThrowIfFailed(sourceBlob->GetEncoding(&known, &encoding));
    DxcBuffer sourceBuffer { .Ptr = sourceBlob->GetBufferPointer(), .Size = sourceBlob->GetBufferSize(), .Encoding = encoding };

    LPCWSTR args[] = {
        shaderPath.c_str(),
        L"-E", entryPoint.c_str(),
        L"-T", targetProfile.c_str(),
        L"-Zi",
        L"-Fd", L"./",
    };

    ComPtr<IDxcResult> result;
    ThrowIfFailed(compiler->Compile(&sourceBuffer, args, _countof(args), includeHandler.Get(), IID_PPV_ARGS(&result)));

    HRESULT hr = S_OK;
    ThrowIfFailed(result->GetStatus(&hr));
    if (FAILED(hr))
    {
        ComPtr<IDxcBlobEncoding> error;
        result->GetErrorBuffer(&error);
        if (error && error->GetBufferSize() > 0)
        {
            const std::string errorMessage(static_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize());
            if (debugLabel)
                std::cerr << debugLabel << " compile error: " << errorMessage << std::endl;
            else
                std::cerr << errorMessage << std::endl;
        }
        ThrowIfFailed(hr);
    }

    ComPtr<IDxcBlob> blob;
    result->GetResult(&blob);
    WriteShaderPdb(result.Get());
    return blob;
}

DELTA_ENGINE_NS_END
