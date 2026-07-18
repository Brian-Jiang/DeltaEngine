#include "Editor/Commands/EditorCommand_AddPostProcessPass.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/EditorCore.h"

#include "Runtime/Graphics/PostProcess/PA_PostProcessStack.h"
#include "Runtime/Graphics/PostProcess/PostProcessPass.h"
#include "Runtime/Graphics/PostProcess/PostProcessStack.h"
#include "Runtime/Serialization/ObjectSnapshotReader.h"
#include "Runtime/Serialization/ObjectSnapshotWriter.h"

#include <algorithm>
#include <format>

using namespace DeltaEngine;

namespace
{

PA_PostProcessStack* ResolvePPStackAsset(EditorCore& core, const AssetId& assetId)
{
    EditorAssetDatabase* db = core.GetAssetDatabase();
    if (!db || assetId.IsNull())
        return nullptr;

    DPrimaryAsset* asset = db->LoadAsset(assetId);
    return dynamic_cast<PA_PostProcessStack*>(asset);
}

} // namespace

EditorCommand_AddPostProcessPass::EditorCommand_AddPostProcessPass(AssetId assetId, std::string className)
    : m_assetId(assetId)
    , m_className(std::move(className))
{
}

std::string_view EditorCommand_AddPostProcessPass::GetDescription() const
{
    if (m_description.empty())
        m_description = std::format("Add PostProcess Pass: {}", m_className);
    return m_description;
}

bool EditorCommand_AddPostProcessPass::Execute(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Add PostProcess Pass] Execute: Start");

    PA_PostProcessStack* asset = ResolvePPStackAsset(ctx.core, m_assetId);
    if (!asset || !asset->GetStack())
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
            "[Add PostProcess Pass] Execute: PA_PostProcessStack not found for assetId '{}'",
            m_assetId.ToString());
        return false;
    }

    PostProcessPass* pass = asset->AddPass(m_className);
    if (!pass)
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
            "[Add PostProcess Pass] Execute: AddPass('{}') failed",
            m_className);
        return false;
    }

    m_createdId = pass->GetObjectId();
    m_passIndex = static_cast<int>(asset->GetStack()->m_passes.size()) - 1;
    asset->MarkDirty();
    return true;
}

bool EditorCommand_AddPostProcessPass::Undo(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Add PostProcess Pass] Undo: Start");

    PA_PostProcessStack* asset = ResolvePPStackAsset(ctx.core, m_assetId);
    if (!asset || !asset->GetStack())
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
            "[Add PostProcess Pass] Undo: PA_PostProcessStack not found for assetId '{}'",
            m_assetId.ToString());
        return false;
    }

    DObject* obj = asset->FindObject(m_createdId);
    PostProcessPass* pass = dynamic_cast<PostProcessPass*>(obj);
    if (!pass)
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
            "[Add PostProcess Pass] Undo: pass '{}' not found in asset",
            m_createdId.ToString());
        return false;
    }

    ObjectSnapshotWriter writer;
    m_snapshot = writer.Capture(pass);
    m_hasSnapshot = true;

    const auto& passes = asset->GetStack()->m_passes;
    auto it = std::find(passes.begin(), passes.end(), pass);
    m_passIndex = (it != passes.end()) ? static_cast<int>(it - passes.begin()) : m_passIndex;

    if (!asset->RemovePass(m_className))
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
            "[Add PostProcess Pass] Undo: RemovePass('{}') failed",
            m_className);
        return false;
    }

    ctx.core.NotifyObjectDestroyed(m_createdId);
    return true;
}

bool EditorCommand_AddPostProcessPass::Redo(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Add PostProcess Pass] Redo: Start");

    if (!m_hasSnapshot)
        return Execute(ctx);

    PA_PostProcessStack* asset = ResolvePPStackAsset(ctx.core, m_assetId);
    if (!asset || !asset->GetStack())
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
            "[Add PostProcess Pass] Redo: PA_PostProcessStack not found for assetId '{}'",
            m_assetId.ToString());
        return false;
    }

    ObjectSnapshotReader reader;
    DObject* restored = reader.Restore(m_snapshot, nullptr, ctx.core.GetAssetDatabase(), asset, nullptr);
    if (!restored)
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
            "[Add PostProcess Pass] Redo: Failed to restore pass from snapshot");
        return false;
    }

    auto* pass = dynamic_cast<PostProcessPass*>(restored);
    if (!pass)
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
            "[Add PostProcess Pass] Redo: Restored object is not a PostProcessPass");
        return false;
    }

    m_createdId = pass->GetObjectId();
    auto& passes = asset->GetStack()->m_passes;
    const int insertAt = (m_passIndex >= 0 && m_passIndex <= static_cast<int>(passes.size()))
        ? m_passIndex
        : static_cast<int>(passes.size());
    passes.insert(passes.begin() + insertAt, pass);
    asset->MarkDirty();
    return true;
}

void EditorCommand_AddPostProcessPass::Serialize(nlohmann::json& out) const
{
    out["assetId"] = m_assetId.ToString();
    out["className"] = m_className;
    out["createdId"] = m_createdId.ToString();
    out["passIndex"] = m_passIndex;

    if (m_hasSnapshot)
    {
        out["snapshot"] = m_snapshot.rootJson;
        out["rootClassName"] = m_snapshot.rootClassName;
        nlohmann::json ids = nlohmann::json::array();
        for (const auto& id : m_snapshot.capturedIds)
            ids.push_back(id.ToString());
        out["capturedIds"] = ids;
    }
}

void EditorCommand_AddPostProcessPass::Deserialize(const nlohmann::json& in)
{
    m_assetId = UUID::FromString(in.value("assetId", ""));
    m_className = in.value("className", "");
    m_createdId = UUID::FromString(in.value("createdId", ""));
    m_passIndex = in.value("passIndex", -1);

    if (in.contains("snapshot"))
    {
        m_snapshot.rootJson = in["snapshot"];
        m_snapshot.rootClassName = in.value("rootClassName", "");
        m_snapshot.capturedIds.clear();
        if (in.contains("capturedIds"))
            for (const auto& idStr : in["capturedIds"])
                m_snapshot.capturedIds.push_back(UUID::FromString(idStr.get<std::string>()));
        m_hasSnapshot = true;
    }

    m_description.clear();
}
