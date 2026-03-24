#include "EditorCommandManager.h"
#include "EditorCommandRegistry.h"

using namespace DeltaEngine;

bool EditorCommandManager::Execute(std::unique_ptr<EditorCommand> cmd, EditorCommandContext& ctx)
{
    if (!cmd->Execute(ctx))
        return false;

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
        return false;

    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    bool result = cmd->Undo(ctx);
    m_redoStack.push_back(std::move(cmd));
    return result;
}

bool EditorCommandManager::Redo(EditorCommandContext& ctx)
{
    if (m_redoStack.empty())
        return false;

    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    bool result = cmd->Redo(ctx);
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
    }
}

void EditorCommandManager::Clear()
{
    m_undoStack.clear();
    m_redoStack.clear();
}
