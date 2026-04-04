#include "EditorCommandManager.h"
#include "EditorCommandRegistry.h"

using namespace DeltaEngine;

bool EditorCommandManager::Execute(std::unique_ptr<EditorCommand> cmd, EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Log, "[Command Manager] Execute: {}", cmd->GetTypeName());
    if (!cmd->Execute(ctx))
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Command Manager] Execute failed: {}", cmd->GetTypeName());
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
    cmd->Execute(ctx);
}

bool EditorCommandManager::Undo(EditorCommandContext& ctx)
{
    if (m_undoStack.empty())
    {
        DLOG(LogEditorCommand, ELogLevel::Warning, "[Command Manager] Undo: Stack empty");
        return false;
    }

    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    DLOG(LogEditorCommand, ELogLevel::Log, "[Command Manager] Undo: {}", cmd->GetTypeName());
    bool result = cmd->Undo(ctx);
    if (!result)
        DLOG(LogEditorCommand, ELogLevel::Error, "[Command Manager] Undo failed: {}", cmd->GetTypeName());
    m_redoStack.push_back(std::move(cmd));
    return result;
}

bool EditorCommandManager::Redo(EditorCommandContext& ctx)
{
    if (m_redoStack.empty())
    {
        DLOG(LogEditorCommand, ELogLevel::Warning, "[Command Manager] Redo: Stack empty");
        return false;
    }

    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    DLOG(LogEditorCommand, ELogLevel::Log, "[Command Manager] Redo: {}", cmd->GetTypeName());
    bool result = cmd->Redo(ctx);
    if (!result)
        DLOG(LogEditorCommand, ELogLevel::Error, "[Command Manager] Redo failed: {}", cmd->GetTypeName());
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
        return;
    for (const auto& envelope : in["undoStack"])
    {
        std::string type = envelope.value("type", "");
        auto cmd = EditorCommandRegistry::Get().Create(type);
        if (cmd)
        {
            cmd->Deserialize(envelope["data"]);
            Execute(std::move(cmd), ctx);
        }
        else
            DLOG(LogEditorCommand, ELogLevel::Error, "[Command Manager] DeserializeAndReplay: Unknown command type: {}", type);
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
