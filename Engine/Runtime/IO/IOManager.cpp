#include "Runtime/IO/IOManager.h"
#include "Runtime/Logging/LogChannels.h"

#include <windows.h>

#include <system_error>

using namespace DeltaEngine;

namespace
{
constexpr const char* k_engineSourceAssetsRel  = "Engine/EngineSourceAssets";
constexpr const char* k_editorSourceAssetsRel  = "Engine/EditorSourceAssets";
constexpr const char* k_engineImportedAssetsRel = "Engine/EngineImportedAssets";
constexpr const char* k_intermediateRel         = "Intermediate";
constexpr const char* k_toolsRel                = "Tools";
constexpr const char* k_settingsRel             = "Settings";
constexpr const char* k_engineSettingsFileName  = "EngineSettings.json";

std::filesystem::path ResolveProjectRoot()
{
    wchar_t buffer[MAX_PATH] = {};
    const DWORD len = ::GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (len == 0 || len == MAX_PATH)
    {
        DLOG(LogIO, ELogLevel::Error,
            "IOManager: GetModuleFileNameW failed (len={}, GetLastError={}); falling back to CWD",
            len, ::GetLastError());
        std::error_code ec;
        auto cwd = std::filesystem::current_path(ec);
        if (ec)
        {
            DLOG(LogIO, ELogLevel::Error,
                "IOManager: current_path also failed: {}", ec.message());
            return std::filesystem::path {};
        }
        return cwd;
    }

    std::filesystem::path exe(buffer);
    // Build/x64-Debug/bin/Foo.exe -> walk up three to repo root.
    std::filesystem::path root = exe.parent_path().parent_path().parent_path().parent_path();

    std::error_code ec;
    if (!std::filesystem::exists(root, ec) || ec)
    {
        DLOG(LogIO, ELogLevel::Warning,
            "IOManager: resolved project root '{}' does not exist (exe='{}'); using CWD",
            root.string(), exe.string());
        auto cwd = std::filesystem::current_path(ec);
        if (ec)
        {
            DLOG(LogIO, ELogLevel::Error,
                "IOManager: current_path failed: {}", ec.message());
            return std::filesystem::path {};
        }
        return cwd;
    }

    DLOG(LogIO, ELogLevel::Display, "IOManager: project root resolved to '{}'", root.string());
    return root;
}

std::filesystem::path JoinUnderRoot(const char* relSegment, const std::filesystem::path& assetName)
{
    DELTA_ENSURE_MSG(!assetName.empty(),
        "IOManager: empty asset name passed to '{}'", relSegment);
    DLOG_IF(LogIO, ELogLevel::Warning, assetName.is_absolute(),
        "IOManager: asset name '{}' is absolute; expected relative",
        assetName.string());
    return IOManager::GetProjectRoot() / relSegment / assetName;
}
}

const std::filesystem::path& IOManager::GetProjectRoot()
{
    static const std::filesystem::path s_root = ResolveProjectRoot();
    return s_root;
}

std::filesystem::path IOManager::GetEngineSourceAssetFullPath(const std::filesystem::path& assetName)
{
    return JoinUnderRoot(k_engineSourceAssetsRel, assetName);
}

std::filesystem::path IOManager::GetEditorSourceAssetFullPath(const std::filesystem::path& assetName)
{
    return JoinUnderRoot(k_editorSourceAssetsRel, assetName);
}

std::filesystem::path IOManager::GetEngineImportedAssetsFolder()
{
    return GetProjectRoot() / k_engineImportedAssetsRel;
}

std::filesystem::path IOManager::GetEngineImportedAssetFullPath(const std::filesystem::path& assetName, bool isJson)
{
    DELTA_ENSURE_MSG(!assetName.empty(),
        "IOManager::GetEngineImportedAssetFullPath: empty asset name");
    DLOG_IF(LogIO, ELogLevel::Warning, assetName.is_absolute(),
        "IOManager::GetEngineImportedAssetFullPath: asset name '{}' is absolute; expected relative",
        assetName.string());

    const char* extension = isJson ? ".dasset.json" : ".dasset";
    std::filesystem::path full = GetProjectRoot() / k_engineImportedAssetsRel / assetName;
    full += extension;
    return full;
}

std::filesystem::path IOManager::GetIntermediateFolder()
{
    return GetProjectRoot() / k_intermediateRel;
}

std::filesystem::path IOManager::GetToolsFolder()
{
    return GetProjectRoot() / k_toolsRel;
}

std::filesystem::path IOManager::GetSettingsFolder()
{
    return GetProjectRoot() / k_settingsRel;
}

std::filesystem::path IOManager::GetEngineSettingsPath()
{
    return GetSettingsFolder() / k_engineSettingsFileName;
}

namespace
{
std::filesystem::path& EditorStateFolderOverride()
{
    static std::filesystem::path s_override;
    return s_override;
}
}

std::filesystem::path IOManager::GetEditorStateFolder()
{
    const std::filesystem::path& overridePath = EditorStateFolderOverride();
    if (!overridePath.empty())
        return overridePath;
    return GetIntermediateFolder() / "EditorState";
}

void IOManager::SetEditorStateFolderOverride(std::filesystem::path path)
{
    EditorStateFolderOverride() = std::move(path);
}

void IOManager::ClearEditorStateFolderOverride()
{
    EditorStateFolderOverride().clear();
}
