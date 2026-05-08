#include "Editor/Commands/EditorCommandManager.h"

#include "Editor/Commands/EditorCommandRegistry.h"

using namespace DeltaEngine;

bool EditorCommandManager::Execute(std::unique_ptr<EditorCommand> cmd, EditorCommandContext& ctx)
{
    DELTA_ASSERT(cmd != nullptr);
    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Command Manager] Execute: {}", cmd->GetTypeName());
    if (!cmd->Execute(ctx))
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[Command Manager] Execute failed: command '{}' reported failure (Undo stack unchanged; fix state manually if persisted)",
             cmd->GetTypeName());
        return false;
    }

    m_redoStack.clear();
    m_undoStack.push_back(std::move(cmd));

    if (static_cast<int>(m_undoStack.size()) > k_maxUndoDepth)
        m_undoStack.erase(m_undoStack.begin());

    return true;
}

void EditorCommandManager::ExecuteAuxiliary(std::unique_ptr<EditorAuxiliaryCommand> cmd, EditorCommandContext& ctx)
{
    DELTA_ASSERT(cmd != nullptr);
    cmd->Execute(ctx);
}

bool EditorCommandManager::Undo(EditorCommandContext& ctx)
{
    if (m_undoStack.empty())
    {
        DLOG(LogEditorCommand, ELogLevel::Warning, "[Command Manager] Undo invoked with empty stack (expected undoable command)");
        return false;
    }

    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Command Manager] Undo: {}", cmd->GetTypeName());
    bool result = cmd->Undo(ctx);
    if (!result)
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[Command Manager] Undo failed: command '{}' reported failure (object may diverge from last executed state)",
             cmd->GetTypeName());
    m_redoStack.push_back(std::move(cmd));
    return result;
}

bool EditorCommandManager::Redo(EditorCommandContext& ctx)
{
    if (m_redoStack.empty())
    {
        DLOG(LogEditorCommand, ELogLevel::Warning, "[Command Manager] Redo invoked with empty stack");
        return false;
    }

    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Command Manager] Redo: {}", cmd->GetTypeName());
    bool result = cmd->Redo(ctx);
    if (!result)
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[Command Manager] Redo failed: command '{}' reported failure",
             cmd->GetTypeName());
    m_undoStack.push_back(std::move(cmd));
    return result;
}

bool EditorCommandManager::CanUndo() const
{
    return !m_undoStack.empty();
}

bool EditorCommandManager::CanRedo() const
{
    return !m_redoStack.empty();
}

std::string_view EditorCommandManager::GetUndoDescription() const
{
    if (m_undoStack.empty())
        return {};
    return m_undoStack.back()->GetDescription();
}

std::string_view EditorCommandManager::GetRedoDescription() const
{
    if (m_redoStack.empty())
        return {};
    return m_redoStack.back()->GetDescription();
}

void EditorCommandManager::SerializeUndoStack(nlohmann::json& out) const
{
    auto& arr = out["undoStack"];
    arr = nlohmann::json::array();
    for (const auto& cmd : m_undoStack)
    {
        nlohmann::json envelope;
        envelope["type"] = cmd->GetTypeName();
        cmd->Serialize(envelope["data"]);
        arr.push_back(std::move(envelope));
    }
}

void EditorCommandManager::DeserializeAndReplay(const nlohmann::json& in, EditorCommandContext& ctx)
{
    if (!in.contains("undoStack"))
    {
        DLOG(LogEditorCommand, ELogLevel::Verbose,
             "[Command Manager] DeserializeAndReplay: input missing 'undoStack' key — nothing replayed");
        return;
    }
    for (const auto& envelope : in["undoStack"])
    {
        const std::string type = envelope.value("type", "");
        if (type.empty())
        {
            DLOG(LogEditorCommand, ELogLevel::Error,
                 "[Command Manager] DeserializeAndReplay: skipped entry with missing or empty command type "
                 "(expected envelope['type'])");
            continue;
        }
        if (!envelope.contains("data"))
        {
            DLOG(LogEditorCommand, ELogLevel::Error,
                 "[Command Manager] DeserializeAndReplay: skipped entry for '{}' — missing 'data' object with serialized fields",
                 type);
            continue;
        }
        if (!envelope["data"].is_object())
        {
            DLOG(LogEditorCommand, ELogLevel::Error,
                 "[Command Manager] DeserializeAndReplay: skipped entry for '{}' — 'data' must be JSON object "
                 "(actual type discriminator: {})",
                 type, envelope["data"].type_name());
            continue;
        }
        auto cmd = EditorCommandRegistry::Get().Create(type);
        if (!cmd)
            continue;

        try
        {
            cmd->Deserialize(envelope["data"]);
        }
        catch (const std::exception& e)
        {
            DLOG(LogEditorCommand, ELogLevel::Error,
                 "[Command Manager] DeserializeAndReplay: deserialization failed for command '{}': {} (skipped)",
                 type, e.what());
            continue;
        }

        Execute(std::move(cmd), ctx);
    }
}

void EditorCommandManager::Clear()
{
    m_undoStack.clear();
    m_redoStack.clear();
}

size_t EditorCommandManager::GetUndoStackDepth() const
{
    return m_undoStack.size();
}
