#include "Editor/EditorWindows/EditorAssetBrowserPaths.h"

#include "Editor/EditorWindows/EditorWindowsLog.h"

#include <system_error>

using namespace DeltaEngine;

DELTA_ENGINE_NS_BEGIN

std::string GetAssetDisplayNameForBrowser(const std::filesystem::path& path)
{
    return path.stem().stem().string();
}

std::filesystem::path UniqueDirUnderParent(const std::filesystem::path& parent, const std::string& base)
{
    auto candidate = parent / base;
    for (int i = 1; std::filesystem::exists(candidate); ++i)
        candidate = parent / (base + "_" + std::to_string(i));
    return candidate;
}

std::filesystem::path UniqueAssetPathInFolder(const std::filesystem::path& folder, const std::string& base)
{
    auto tryPath = [&](const std::string& name) { return folder / (name + ".dasset.json"); };
    auto candidate = tryPath(base);
    for (int i = 1; std::filesystem::exists(candidate); ++i)
        candidate = tryPath(base + "_" + std::to_string(i));
    return candidate;
}

std::filesystem::path NormalizeEditorPath(const std::filesystem::path& path)
{
    std::error_code ec;
    const auto normalized = std::filesystem::weakly_canonical(path, ec);
    if (ec)
    {
        DLOG(LogEditorWindows, ELogLevel::Verbose, "weakly_canonical failed for '{}', using absolute (message: {})",
            path.string(), ec.message());
        return std::filesystem::absolute(path);
    }
    return normalized;
}

bool IsSameOrChildPathNormalized(const std::filesystem::path& path, const std::filesystem::path& parent)
{
    const auto normPath   = NormalizeEditorPath(path);
    const auto normParent = NormalizeEditorPath(parent);
    if (normPath == normParent)
        return true;

    std::error_code ec;
    const auto rel = std::filesystem::relative(normPath, normParent, ec);
    if (ec || rel.empty())
    {
        if (ec)
            DLOG(LogEditorWindows, ELogLevel::Verbose,
                "relative() failed for child='{}' parent='{}' (expected resolvable hierarchy, message: {})",
                normPath.string(), normParent.string(), ec.message());
        return false;
    }

    const auto first = *rel.begin();
    return first != "..";
}

std::string ToAssetRootRelativeString(const std::filesystem::path& path, const std::filesystem::path& assetRoot)
{
    std::error_code ec;
    auto rel = std::filesystem::relative(NormalizeEditorPath(path), NormalizeEditorPath(assetRoot), ec);
    if (ec)
    {
        DLOG(LogEditorWindows, ELogLevel::Warning,
            "Failed to make path relative to asset root: path='{}' assetRoot='{}' (message: {}, expected both under same root)",
            path.string(), assetRoot.string(), ec.message());
        return {};
    }
    return rel.generic_string();
}

DELTA_ENGINE_NS_END
