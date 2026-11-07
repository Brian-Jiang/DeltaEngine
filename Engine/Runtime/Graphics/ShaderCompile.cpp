#include "Graphics/ShaderCompile.h"

#include "Graphics/DXUtils.h"
#include "IO/IOManager.h"
#include "Runtime/Utils/StringUtils.h"
#include "Runtime/Logging/LogChannels.h"

#include <slang.h>
#include <slang-com-ptr.h>

#include <cstdio>
#include <filesystem>
#include <string>

using namespace DeltaEngine;

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
    {
        const std::filesystem::path pdbPath(pdbName->GetStringPointer());
        DLOG(LogShader, ELogLevel::Warning,
            "Failed to open shader PDB for write: '{}'", pdbPath.string());
        return;
    }

    std::fwrite(pdb->GetBufferPointer(), pdb->GetBufferSize(), 1, file);
    std::fclose(file);
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
    DLOG(LogShader, ELogLevel::Warning, "[{}] slang diagnostics: {}",
        debugLabel ? debugLabel : "", message);
}

/// Accepts "sm_6_6", "vs_6_6", "ps_6_6" etc. Strips the stage prefix if present.
std::string NormalizeSlangProfile(const std::string& targetProfile)
{
    std::string profile = targetProfile;
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
    const std::filesystem::path& engineRelativePath,
    const std::string& entryPoint,
    const std::string& targetProfile,
    const char* debugLabel)
{
    DELTA_ENSURE(!engineRelativePath.empty());
    DELTA_ENSURE(!entryPoint.empty());
    DELTA_ENSURE(!targetProfile.empty());

    ComPtr<IDxcUtils> dxcUtils;
    ComPtr<IDxcCompiler3> compiler;
    ComPtr<IDxcIncludeHandler> includeHandler;
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils)));
    ThrowIfFailed(dxcUtils->CreateDefaultIncludeHandler(&includeHandler));

    const std::wstring shaderPath = IOManager::GetEngineSourceAssetFullPath(engineRelativePath).wstring();
    ComPtr<IDxcBlobEncoding> sourceBlob;
    ThrowIfFailed(dxcUtils->LoadFile(shaderPath.c_str(), nullptr, &sourceBlob));

    BOOL known = FALSE;
    UINT32 encoding = 0;
    ThrowIfFailed(sourceBlob->GetEncoding(&known, &encoding));
    DxcBuffer sourceBuffer { .Ptr = sourceBlob->GetBufferPointer(), .Size = sourceBlob->GetBufferSize(), .Encoding = encoding };

    const std::wstring entryPointW = StringUtils::Utf8ToWString(entryPoint);
    const std::wstring targetProfileW = StringUtils::Utf8ToWString(targetProfile);

    LPCWSTR args[] = {
        shaderPath.c_str(),
        L"-E", entryPointW.c_str(),
        L"-T", targetProfileW.c_str(),
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
            DLOG(LogShader, ELogLevel::Error, "[{}] HLSL compile error: {}",
                debugLabel ? debugLabel : "", errorMessage);
        }
        ThrowIfFailed(hr);
    }

    ComPtr<IDxcBlob> blob;
    result->GetResult(&blob);
    WriteShaderPdb(result.Get());
    return blob;
}

Slang::ComPtr<ISlangBlob> CompileSlangStage(
    const std::filesystem::path& engineRelativePath,
    const std::string& entryPoint,
    const std::string& targetProfile,
    const char* debugLabel)
{
    DELTA_ENSURE(!engineRelativePath.empty());
    DELTA_ENSURE(!entryPoint.empty());
    DELTA_ENSURE(!targetProfile.empty());

    slang::IGlobalSession* globalSession = GetSlangGlobalSession().get();
    if (!globalSession)
    {
        DLOG(LogShader, ELogLevel::Error, "[{}] slang: failed to create global session for '{}'",
            debugLabel ? debugLabel : "", engineRelativePath.string());
        return {};
    }

    const std::filesystem::path fsPath = IOManager::GetEngineSourceAssetFullPath(engineRelativePath);
    const std::u8string parentU8 = fsPath.parent_path().u8string();
    const std::string searchPath(reinterpret_cast<const char*>(parentU8.data()), parentU8.size());
    const std::u8string stemU8 = fsPath.stem().u8string();
    const std::string moduleName(reinterpret_cast<const char*>(stemU8.data()), stemU8.size());

    slang::TargetDesc target {};
    target.format = SLANG_DXIL;
    target.profile = globalSession->findProfile(NormalizeSlangProfile(targetProfile).c_str());

#if defined(_DEBUG)
    // Embed DXIL debug info + source into the shader blob so PIX/RenderDoc auto-load
    // source-level debugging straight from a capture (no PDB search path needed).
    slang::CompilerOptionEntry debugOptions[] = {
        { slang::CompilerOptionName::DebugInformation,
            { slang::CompilerOptionValueKind::Int, SLANG_DEBUG_INFO_LEVEL_MAXIMAL } },
        { slang::CompilerOptionName::Optimization,
            { slang::CompilerOptionValueKind::Int, SLANG_OPTIMIZATION_LEVEL_NONE } },
    };
    target.compilerOptionEntries = debugOptions;
    target.compilerOptionEntryCount = static_cast<uint32_t>(_countof(debugOptions));
#endif

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
        DLOG(LogShader, ELogLevel::Error, "[{}] slang: failed to create session for '{}'",
            debugLabel ? debugLabel : "", engineRelativePath.string());
        return {};
    }

    Slang::ComPtr<ISlangBlob> diagnostics;
    slang::IModule* module = session->loadModule(moduleName.c_str(), diagnostics.writeRef());
    LogSlangDiagnostics(diagnostics.get(), debugLabel);
    if (!module)
    {
        DLOG(LogShader, ELogLevel::Error, "[{}] slang: failed to load module '{}' (path '{}')",
            debugLabel ? debugLabel : "", moduleName, engineRelativePath.string());
        return {};
    }

    Slang::ComPtr<slang::IEntryPoint> entryPointObj;
    if (SLANG_FAILED(module->findEntryPointByName(entryPoint.c_str(), entryPointObj.writeRef())) || !entryPointObj)
    {
        DLOG(LogShader, ELogLevel::Error, "[{}] slang: entry point '{}' not found in '{}'",
            debugLabel ? debugLabel : "", entryPoint, engineRelativePath.string());
        return {};
    }

    slang::IComponentType* components[] = { module, entryPointObj.get() };
    Slang::ComPtr<slang::IComponentType> composite;
    diagnostics = nullptr;
    if (SLANG_FAILED(session->createCompositeComponentType(components, 2, composite.writeRef(), diagnostics.writeRef())))
    {
        LogSlangDiagnostics(diagnostics.get(), debugLabel);
        DLOG(LogShader, ELogLevel::Error, "[{}] slang: failed to create composite for '{}' entry='{}'",
            debugLabel ? debugLabel : "", engineRelativePath.string(), entryPoint);
        return {};
    }
    LogSlangDiagnostics(diagnostics.get(), debugLabel);

    Slang::ComPtr<slang::IComponentType> linked;
    diagnostics = nullptr;
    if (SLANG_FAILED(composite->link(linked.writeRef(), diagnostics.writeRef())))
    {
        LogSlangDiagnostics(diagnostics.get(), debugLabel);
        DLOG(LogShader, ELogLevel::Error, "[{}] slang: link failed for '{}' entry='{}'",
            debugLabel ? debugLabel : "", engineRelativePath.string(), entryPoint);
        return {};
    }
    LogSlangDiagnostics(diagnostics.get(), debugLabel);

    Slang::ComPtr<ISlangBlob> code;
    diagnostics = nullptr;
    if (SLANG_FAILED(linked->getEntryPointCode(0, 0, code.writeRef(), diagnostics.writeRef())))
    {
        LogSlangDiagnostics(diagnostics.get(), debugLabel);
        DLOG(LogShader, ELogLevel::Error, "[{}] slang: getEntryPointCode failed for '{}' entry='{}'",
            debugLabel ? debugLabel : "", engineRelativePath.string(), entryPoint);
        return {};
    }
    LogSlangDiagnostics(diagnostics.get(), debugLabel);

    DLOG(LogShader, ELogLevel::Verbose, "[{}] slang: compiled '{}' entry='{}' profile='{}'",
        debugLabel ? debugLabel : "", engineRelativePath.string(), entryPoint, targetProfile);
    return code;
}

DELTA_ENGINE_NS_END
