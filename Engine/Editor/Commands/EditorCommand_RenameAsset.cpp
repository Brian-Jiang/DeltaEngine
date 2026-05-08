#include "Editor/Commands/EditorCommand_RenameAsset.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/EditorCore.h"

#include <filesystem>

using namespace DeltaEngine;

EditorCommand_RenameAsset::EditorCommand_RenameAsset(AssetId assetId, std::string desiredStem)
    : m_assetId(assetId)
    , m_desiredStem(std::move(desiredStem))
{
}

std::string_view EditorCommand_RenameAsset::GetDescription() const
{
    if (m_description.empty())
    {
        if (!m_newStem.empty())
            m_description = "Rename asset to " + m_newStem;
        else if (!m_desiredStem.empty())
            m_description = "Rename asset to " + m_desiredStem;
        else
            m_description = "Rename asset";
    }
    return m_description;
}

bool EditorCommand_RenameAsset::Execute(EditorCommandContext& ctx)
{
    EditorAssetDatabase* db = ctx.core.GetAssetDatabase();
    if (!db)
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[Rename Asset] Execute failed for asset '{}': EditorAssetDatabase is null (editor not initialized?)",
             m_assetId.ToString());
        return false;
    }

    const std::filesystem::path path = db->GetAssetPath(m_assetId);
    if (path.empty())
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[Rename Asset] Execute failed for asset '{}': registry has no filesystem path "
             "(expected registered .dasset.json)",
             m_assetId.ToString());
        return false;
    }

    if (!path.string().ends_with(".dasset.json"))
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[Rename Asset] Execute rejected for asset '{}': resolved path '{}' does not end with '.dasset.json' "
             "(expected primary asset wrapper)",
             m_assetId.ToString(),
             path.generic_string());
        return false;
    }

    const std::string currentStem = path.stem().stem().string();

    if (!m_newStem.empty() && currentStem == m_newStem)
        return true;

    if (m_newStem.empty())
    {
        m_oldStem = currentStem;
        std::string finalStem;
        if (!db->RenameAssetToStem(m_assetId, m_desiredStem, &finalStem))
        {
            DLOG(LogEditorCommand, ELogLevel::Error,
                 "[Rename Asset] RenameAssetToStem failed for asset '{}' from '{}' to desired stem '{}' "
                 "(path was '{}'; target name may be taken or filesystem error)",
                 m_assetId.ToString(),
                 currentStem,
                 m_desiredStem,
                 path.generic_string());
            return false;
        }
        m_newStem = std::move(finalStem);
        m_description.clear();
        return true;
    }

    if (!db->RenameAssetToExactStem(m_assetId, m_newStem))
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[Rename Asset] RenameAssetToExactStem failed for asset '{}' to '{}.dasset.json' "
             "(file may exist or filesystem error)",
             m_assetId.ToString(),
             m_newStem);
        return false;
    }
    return true;
}

bool EditorCommand_RenameAsset::Undo(EditorCommandContext& ctx)
{
    EditorAssetDatabase* db = ctx.core.GetAssetDatabase();
    if (!db || m_oldStem.empty() || m_newStem.empty())
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[Rename Asset] Undo prerequisites failed — db={}, oldStem nonempty={}, newStem nonempty={}",
             db != nullptr,
             !m_oldStem.empty(),
             !m_newStem.empty());
        return false;
    }

    if (!db->RenameAssetToExactStem(m_assetId, m_oldStem))
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[Rename Asset] Undo failed to rename asset '{}' back to stem '{}' (currently '{}')",
             m_assetId.ToString(),
             m_oldStem,
             m_newStem);
        return false;
    }

    m_description.clear();
    return true;
}

void EditorCommand_RenameAsset::Serialize(nlohmann::json& out) const
{
    out["assetId"]     = m_assetId.ToString();
    out["desiredStem"] = m_desiredStem;
    out["oldStem"]     = m_oldStem;
    out["newStem"]     = m_newStem;
}

void EditorCommand_RenameAsset::Deserialize(const nlohmann::json& in)
{
    m_assetId     = UUID::FromString(in.value("assetId", ""));
    m_desiredStem = in.value("desiredStem", "");
    m_oldStem     = in.value("oldStem", "");
    m_newStem     = in.value("newStem", "");
    m_description.clear();
}
