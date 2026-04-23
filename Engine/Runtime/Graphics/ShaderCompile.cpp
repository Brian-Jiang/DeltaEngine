#include "Graphics/ShaderCompile.h"

#include "Graphics/DXUtils.h"
#include "IO/IOManager.h"

#include <slang.h>
#include <slang-com-ptr.h>

#include <cstdio>
#include <filesystem>
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

std::string WideToUtf8(const std::wstring& wide)
{
    if (wide.empty())
        return {};
    const int size = ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    std::string out(static_cast<size_t>(size), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), out.data(), size, nullptr, nullptr);
    return out;
}

Slang::ComPtr<slang::IGlobalSession>& GetSlangGlobalSession()
{
    static Slang::ComPtr<slang::IGlobalSession> session = []
    {
        Slang::ComPtr<slang::IGlobalSession> s;
        slang::createGlobalSession(s.writeRef());
        return s;
    }();
    return session;
}

void LogSlangDiagnostics(ISlangBlob* diagnostics, const char* debugLabel)
{
    if (!diagnostics || diagnostics->getBufferSize() == 0)
        return;
    const std::string message(
        static_cast<const char*>(diagnostics->getBufferPointer()),
        diagnostics->getBufferSize());
    if (debugLabel)
        std::cerr << debugLabel << " slang diagnostics: " << message << std::endl;
    else
        std::cerr << message << std::endl;
}

/// Accepts "sm_6_6", "vs_6_6", "ps_6_6" etc. Strips the stage prefix if present.
std::string NormalizeSlangProfile(const std::wstring& targetProfile)
{
    std::string profile = WideToUtf8(targetProfile);
    if (profile.size() > 3 && profile[2] == '_')
    {
        const char c0 = profile[0];
        const char c1 = profile[1];
        const bool isStagePrefix =
            (c0 == 'v' && c1 == 's') || (c0 == 'p' && c1 == 's') ||
            (c0 == 'c' && c1 == 's') || (c0 == 'g' && c1 == 's') ||
            (c0 == 'h' && c1 == 's') || (c0 == 'd' && c1 == 's');
        if (isStagePrefix)
            profile = "sm" + profile.substr(2);
    }
    return profile;
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

Slang::ComPtr<ISlangBlob> CompileSlangStage(
    const std::wstring& engineRelativePath,
    const std::wstring& entryPoint,
    const std::wstring& targetProfile,
    const char* debugLabel)
{
    slang::IGlobalSession* globalSession = GetSlangGlobalSession().get();
    if (!globalSession)
    {
        std::cerr << (debugLabel ? debugLabel : "") << " slang: failed to create global session" << std::endl;
        return {};
    }

    const std::wstring fullPath = IOManager::GetEngineSourceAssetFullPath(engineRelativePath);
    const std::filesystem::path fsPath(fullPath);
    const std::string searchPath = WideToUtf8(fsPath.parent_path().wstring());
    const std::string moduleName = WideToUtf8(fsPath.stem().wstring());
    const std::string entryPointUtf8 = WideToUtf8(entryPoint);

    slang::TargetDesc target {};
    target.format = SLANG_DXIL;
    target.profile = globalSession->findProfile(NormalizeSlangProfile(targetProfile).c_str());

    const char* searchPaths[] = { searchPath.c_str() };

    slang::SessionDesc sessionDesc {};
    sessionDesc.targets = &target;
    sessionDesc.targetCount = 1;
    sessionDesc.searchPaths = searchPaths;
    sessionDesc.searchPathCount = 1;
    sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_ROW_MAJOR;

    Slang::ComPtr<slang::ISession> session;
    if (SLANG_FAILED(globalSession->createSession(sessionDesc, session.writeRef())))
    {
        std::cerr << (debugLabel ? debugLabel : "") << " slang: failed to create session" << std::endl;
        return {};
    }

    Slang::ComPtr<ISlangBlob> diagnostics;
    slang::IModule* module = session->loadModule(moduleName.c_str(), diagnostics.writeRef());
    LogSlangDiagnostics(diagnostics.get(), debugLabel);
    if (!module)
        return {};

    Slang::ComPtr<slang::IEntryPoint> entryPointObj;
    if (SLANG_FAILED(module->findEntryPointByName(entryPointUtf8.c_str(), entryPointObj.writeRef())) || !entryPointObj)
    {
        std::cerr << (debugLabel ? debugLabel : "") << " slang: entry point '" << entryPointUtf8 << "' not found" << std::endl;
        return {};
    }

    slang::IComponentType* components[] = { module, entryPointObj.get() };
    Slang::ComPtr<slang::IComponentType> composite;
    diagnostics = nullptr;
    if (SLANG_FAILED(session->createCompositeComponentType(components, 2, composite.writeRef(), diagnostics.writeRef())))
    {
        LogSlangDiagnostics(diagnostics.get(), debugLabel);
        return {};
    }
    LogSlangDiagnostics(diagnostics.get(), debugLabel);

    Slang::ComPtr<slang::IComponentType> linked;
    diagnostics = nullptr;
    if (SLANG_FAILED(composite->link(linked.writeRef(), diagnostics.writeRef())))
    {
        LogSlangDiagnostics(diagnostics.get(), debugLabel);
        return {};
    }
    LogSlangDiagnostics(diagnostics.get(), debugLabel);

    Slang::ComPtr<ISlangBlob> code;
    diagnostics = nullptr;
    if (SLANG_FAILED(linked->getEntryPointCode(0, 0, code.writeRef(), diagnostics.writeRef())))
    {
        LogSlangDiagnostics(diagnostics.get(), debugLabel);
        return {};
    }
    LogSlangDiagnostics(diagnostics.get(), debugLabel);

    return code;
}

DELTA_ENGINE_NS_END
