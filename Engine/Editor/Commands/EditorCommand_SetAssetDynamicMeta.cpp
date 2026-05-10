#include "Editor/Commands/EditorCommand_SetAssetDynamicMeta.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/EditorCore.h"

#include "Runtime/Assets/DPrimaryAsset.h"

#include <optional>

using namespace DeltaEngine;

namespace
{
constexpr std::string_view kStaticPrefix = "/static";
constexpr std::string_view kDynamicRoot  = "/dynamic";
constexpr std::string_view kDynamicChild = "/dynamic/";

std::optional<nlohmann::json::json_pointer> TranslatePointer(const std::string& path)
{
    if (path.empty() || path == kDynamicRoot)
        return nlohmann::json::json_pointer{};

    if (path.starts_with(kDynamicChild))
    {
        std::string inner = path.substr(kDynamicRoot.size());
        try
        {
            return nlohmann::json::json_pointer{inner};
        }
        catch (const std::exception&)
        {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

bool IsStaticPath(const std::string& path)
{
    return path == kStaticPrefix || path.starts_with(std::string(kStaticPrefix) + "/");
}

bool IsValidDynamicMetaShape(const nlohmann::json& meta)
{
    if (!meta.is_object())
        return false;

    auto descIt = meta.find("desc");
    if (descIt == meta.end() || !descIt->is_string())
        return false;

    auto tagsIt = meta.find("tags");
    if (tagsIt == meta.end() || !tagsIt->is_array())
        return false;

    for (const auto& tag : *tagsIt)
    {
        if (!tag.is_string())
            return false;
    }

    return true;
}

bool EraseAtPointer(nlohmann::json& root, const nlohmann::json::json_pointer& jp)
{
    if (jp.empty())
        return false;

    auto parentJp = jp.parent_pointer();
    nlohmann::json* parent = nullptr;
    try
    {
        parent = &root.at(parentJp);
    }
    catch (const std::exception&)
    {
        return false;
    }

    const std::string& last = jp.back();
    if (parent->is_object())
    {
        parent->erase(last);
        return true;
    }
    if (parent->is_array())
    {
        try
        {
            const size_t idx = std::stoul(last);
            if (idx >= parent->size())
                return false;
            parent->erase(idx);
            return true;
        }
        catch (const std::exception&)
        {
            return false;
        }
    }
    return false;
}
} // namespace

EditorCommand_SetAssetDynamicMeta::EditorCommand_SetAssetDynamicMeta(
    AssetId assetId, std::string jsonPointer, nlohmann::json newValue)
    : m_assetId(assetId)
    , m_jsonPointer(std::move(jsonPointer))
    , m_valueAfter(std::move(newValue))
{
}

std::string_view EditorCommand_SetAssetDynamicMeta::GetDescription() const
{
    if (m_description.empty())
    {
        const std::string& shown = m_jsonPointer.empty() ? std::string(kDynamicRoot) : m_jsonPointer;
        m_description = "Set meta " + shown;
    }
    return m_description;
}

bool EditorCommand_SetAssetDynamicMeta::ApplyMutation(
    EditorCommandContext& ctx, const nlohmann::json& value, bool valuePresent)
{
    EditorAssetDatabase* db = ctx.core.GetAssetDatabase();
    if (!db)
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[Set Dynamic Meta] EditorAssetDatabase unavailable for asset '{}'",
             m_assetId.ToString());
        return false;
    }

    DPrimaryAsset* asset = db->GetLoadedAsset(m_assetId);
    if (!asset)
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[Set Dynamic Meta] Asset '{}' is not loaded (expected loaded instance)",
             m_assetId.ToString());
        return false;
    }

    if (IsStaticPath(m_jsonPointer))
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[Set Dynamic Meta] Rejected: path '{}' targets static meta (read-only) on asset '{}'",
             m_jsonPointer, m_assetId.ToString());
        return false;
    }

    auto innerOpt = TranslatePointer(m_jsonPointer);
    if (!innerOpt)
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[Set Dynamic Meta] Rejected: invalid path '{}' (must be '', '/dynamic', or '/dynamic/...') for asset '{}'",
             m_jsonPointer, m_assetId.ToString());
        return false;
    }
    const auto& innerJp = *innerOpt;

    nlohmann::json candidate = asset->GetDynamicMeta();

    if (innerJp.empty())
    {
        if (!valuePresent)
        {
            DLOG(LogEditorCommand, ELogLevel::Error,
                 "[Set Dynamic Meta] Cannot erase whole dynamic blob (asset '{}')",
                 m_assetId.ToString());
            return false;
        }
        candidate = value;
    }
    else if (valuePresent)
    {
        try
        {
            candidate[innerJp] = value;
        }
        catch (const std::exception& e)
        {
            DLOG(LogEditorCommand, ELogLevel::Error,
                 "[Set Dynamic Meta] Apply at '{}' failed for asset '{}': {}",
                 m_jsonPointer, m_assetId.ToString(), e.what());
            return false;
        }
    }
    else
    {
        if (!EraseAtPointer(candidate, innerJp))
        {
            DLOG(LogEditorCommand, ELogLevel::Error,
                 "[Set Dynamic Meta] Erase at '{}' failed for asset '{}'",
                 m_jsonPointer, m_assetId.ToString());
            return false;
        }
    }

    if (!IsValidDynamicMetaShape(candidate))
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[Set Dynamic Meta] Validation failed for asset '{}' at path '{}': "
             "result must be an object with string 'desc' and string-array 'tags'",
             m_assetId.ToString(), m_jsonPointer);
        return false;
    }

    asset->SetDynamicMeta(std::move(candidate));
    asset->MarkDirty();
    db->RefreshAssetMetaCache(m_assetId);
    return true;
}

bool EditorCommand_SetAssetDynamicMeta::Execute(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Verbose,
         "[Set Dynamic Meta] Execute: asset='{}' path='{}'",
         m_assetId.ToString(), m_jsonPointer);

    if (!m_snapshotTaken)
    {
        EditorAssetDatabase* db = ctx.core.GetAssetDatabase();
        DPrimaryAsset* asset = db ? db->GetLoadedAsset(m_assetId) : nullptr;
        if (asset && !IsStaticPath(m_jsonPointer))
        {
            if (auto innerOpt = TranslatePointer(m_jsonPointer))
            {
                const auto& innerJp = *innerOpt;
                if (innerJp.empty())
                {
                    m_valueBefore = asset->GetDynamicMeta();
                    m_hadValueBefore = true;
                }
                else
                {
                    try
                    {
                        m_valueBefore = asset->GetDynamicMeta().at(innerJp);
                        m_hadValueBefore = true;
                    }
                    catch (const nlohmann::json::out_of_range&)
                    {
                        m_valueBefore = nlohmann::json{};
                        m_hadValueBefore = false;
                    }
                }
                m_snapshotTaken = true;
            }
        }
    }

    return ApplyMutation(ctx, m_valueAfter, /*valuePresent*/true);
}

bool EditorCommand_SetAssetDynamicMeta::Undo(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Verbose,
         "[Set Dynamic Meta] Undo: asset='{}' path='{}'",
         m_assetId.ToString(), m_jsonPointer);
    return ApplyMutation(ctx, m_valueBefore, m_hadValueBefore);
}

void EditorCommand_SetAssetDynamicMeta::Serialize(nlohmann::json& out) const
{
    out["assetId"]        = m_assetId.ToString();
    out["jsonPointer"]    = m_jsonPointer;
    out["valueBefore"]    = m_valueBefore;
    out["valueAfter"]     = m_valueAfter;
    out["hadValueBefore"] = m_hadValueBefore;
    out["snapshotTaken"]  = m_snapshotTaken;
}

void EditorCommand_SetAssetDynamicMeta::Deserialize(const nlohmann::json& in)
{
    m_assetId        = UUID::FromString(in.value("assetId", ""));
    m_jsonPointer    = in.value("jsonPointer", "");
    m_valueBefore    = in.value("valueBefore", nlohmann::json{});
    m_valueAfter     = in.value("valueAfter", nlohmann::json{});
    m_hadValueBefore = in.value("hadValueBefore", false);
    m_snapshotTaken  = in.value("snapshotTaken", false);
    m_description.clear();
}
