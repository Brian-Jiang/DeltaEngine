#include "Editor/Commands/EditorCommand_RemovePostProcessPass.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/EditorCore.h"

#include "Runtime/Graphics/PostProcess/PA_PostProcessStack.h"
#include "Runtime/Graphics/PostProcess/PostProcessPass.h"
#include "Runtime/Graphics/PostProcess/PostProcessStack.h"
#include "Runtime/Reflection/DClass.h"
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

PostProcessPass* FindPassByClass(PostProcessStack* stack, const std::string& className, int& outIndex)
{
    outIndex = -1;
    if (!stack)
        return nullptr;

    for (int i = 0; i < static_cast<int>(stack->m_passes.size()); ++i)
    {
        PostProcessPass* p = stack->m_passes[static_cast<size_t>(i)];
        if (p && p->GetClass() && p->GetClass()->GetName() == className)
        {
            outIndex = i;
            return p;
        }
    }
    return nullptr;
}

} // namespace

EditorCommand_RemovePostProcessPass::EditorCommand_RemovePostProcessPass(AssetId assetId, std::string className)
    : m_assetId(assetId)
    , m_className(std::move(className))
{
}

std::string_view EditorCommand_RemovePostProcessPass::GetDescription() const
{
    if (m_description.empty())
        m_description = std::format("Remove PostProcess Pass: {}", m_className);
    return m_description;
}

bool EditorCommand_RemovePostProcessPass::Execute(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Remove PostProcess Pass] Execute: Start");

    PA_PostProcessStack* asset = ResolvePPStackAsset(ctx.core, m_assetId);
    if (!asset || !asset->m_stack)
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
            "[Remove PostProcess Pass] Execute: PA_PostProcessStack not found for assetId '{}'",
            m_assetId.ToString());
        return false;
    }

    PostProcessPass* pass = FindPassByClass(asset->m_stack, m_className, m_passIndex);
    if (!pass)
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
            "[Remove PostProcess Pass] Execute: no pass of class '{}' in stack",
            m_className);
        return false;
    }

    m_removedId = pass->GetObjectId();

    ObjectSnapshotWriter writer;
    m_snapshot = writer.Capture(pass);

    if (!asset->RemovePass(m_className))
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
            "[Remove PostProcess Pass] Execute: RemovePass('{}') failed",
            m_className);
        return false;
    }

    ctx.core.NotifyObjectDestroyed(m_removedId);
    return true;
}

bool EditorCommand_RemovePostProcessPass::Undo(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Remove PostProcess Pass] Undo: Start");

    PA_PostProcessStack* asset = ResolvePPStackAsset(ctx.core, m_assetId);
    if (!asset || !asset->m_stack)
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
            "[Remove PostProcess Pass] Undo: PA_PostProcessStack not found for assetId '{}'",
            m_assetId.ToString());
        return false;
    }

    ObjectSnapshotReader reader;
    DObject* restored = reader.Restore(m_snapshot, nullptr, ctx.core.GetAssetDatabase(), asset, nullptr);
    if (!restored)
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
            "[Remove PostProcess Pass] Undo: Failed to restore pass from snapshot");
        return false;
    }

    auto* pass = dynamic_cast<PostProcessPass*>(restored);
    if (!pass)
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
            "[Remove PostProcess Pass] Undo: Restored object is not a PostProcessPass");
        return false;
    }

    m_removedId = pass->GetObjectId();
    auto& passes = asset->m_stack->m_passes;
    const int insertAt = (m_passIndex >= 0 && m_passIndex <= static_cast<int>(passes.size()))
        ? m_passIndex
        : static_cast<int>(passes.size());
    passes.insert(passes.begin() + insertAt, pass);
    asset->MarkDirty();
    return true;
}

bool EditorCommand_RemovePostProcessPass::Redo(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Remove PostProcess Pass] Redo: Start");
    return Execute(ctx);
}

void EditorCommand_RemovePostProcessPass::Serialize(nlohmann::json& out) const
{
    out["assetId"] = m_assetId.ToString();
    out["className"] = m_className;
    out["removedId"] = m_removedId.ToString();
    out["passIndex"] = m_passIndex;

    out["snapshot"] = m_snapshot.rootJson;
    out["rootClassName"] = m_snapshot.rootClassName;
    nlohmann::json ids = nlohmann::json::array();
    for (const auto& id : m_snapshot.capturedIds)
        ids.push_back(id.ToString());
    out["capturedIds"] = ids;
}

void EditorCommand_RemovePostProcessPass::Deserialize(const nlohmann::json& in)
{
    m_assetId = UUID::FromString(in.value("assetId", ""));
    m_className = in.value("className", "");
    m_removedId = UUID::FromString(in.value("removedId", ""));
    m_passIndex = in.value("passIndex", -1);

    m_snapshot.rootJson = in.value("snapshot", nlohmann::json{});
    m_snapshot.rootClassName = in.value("rootClassName", "");
    m_snapshot.capturedIds.clear();
    if (in.contains("capturedIds"))
        for (const auto& idStr : in["capturedIds"])
            m_snapshot.capturedIds.push_back(UUID::FromString(idStr.get<std::string>()));

    m_description.clear();
}
