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
        return false;

    const std::filesystem::path path = db->GetAssetPath(m_assetId);
    if (path.empty() || !path.string().ends_with(".dasset.json"))
        return false;

    const std::string currentStem = path.stem().stem().string();

    if (!m_newStem.empty() && currentStem == m_newStem)
        return true;

    if (m_newStem.empty())
    {
        m_oldStem = currentStem;
        std::string finalStem;
        if (!db->RenameAssetToStem(m_assetId, m_desiredStem, &finalStem))
            return false;
        m_newStem = std::move(finalStem);
        m_description.clear();
        return true;
    }

    return db->RenameAssetToExactStem(m_assetId, m_newStem);
}

bool EditorCommand_RenameAsset::Undo(EditorCommandContext& ctx)
{
    EditorAssetDatabase* db = ctx.core.GetAssetDatabase();
    if (!db || m_oldStem.empty() || m_newStem.empty())
        return false;

    if (!db->RenameAssetToExactStem(m_assetId, m_oldStem))
        return false;

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
