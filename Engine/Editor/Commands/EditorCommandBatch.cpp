#include "EditorCommandBatch.h"

using namespace DeltaEngine;

EditorCommandBatch::EditorCommandBatch(std::string description)
    : m_description(std::move(description))
{
}

void EditorCommandBatch::Add(std::unique_ptr<EditorCommand> cmd)
{
    m_commands.push_back(std::move(cmd));
}

bool EditorCommandBatch::Execute(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Log, "[Command Batch] Execute: Start ({} commands)", m_commands.size());
    for (auto& cmd : m_commands)
    {
        if (!cmd->Execute(ctx))
        {
            DLOG(LogEditorCommand, ELogLevel::Error, "[Command Batch] Execute: Sub-command failed: {}", cmd->GetTypeName());
            return false;
        }
    }
    return true;
}

bool EditorCommandBatch::Undo(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Log, "[Command Batch] Undo: Start ({} commands)", m_commands.size());
    for (auto it = m_commands.rbegin(); it != m_commands.rend(); ++it)
    {
        if (!(*it)->Undo(ctx))
        {
            DLOG(LogEditorCommand, ELogLevel::Error, "[Command Batch] Undo: Sub-command failed: {}", (*it)->GetTypeName());
            return false;
        }
    }
    return true;
}

bool EditorCommandBatch::Redo(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Log, "[Command Batch] Redo: Start ({} commands)", m_commands.size());
    for (auto& cmd : m_commands)
    {
        if (!cmd->Redo(ctx))
        {
            DLOG(LogEditorCommand, ELogLevel::Error, "[Command Batch] Redo: Sub-command failed: {}", cmd->GetTypeName());
            return false;
        }
    }
    return true;
}

void EditorCommandBatch::Serialize(nlohmann::json& out) const
{
    out["description"] = m_description;
    auto& arr = out["commands"];
    arr = nlohmann::json::array();
    for (const auto& cmd : m_commands)
    {
        nlohmann::json envelope;
        envelope["type"] = cmd->GetTypeName();
        cmd->Serialize(envelope["data"]);
        arr.push_back(std::move(envelope));
    }
}

void EditorCommandBatch::Deserialize(const nlohmann::json& in)
{
    m_description = in.value("description", "");
    m_commands.clear();
    if (!in.contains("commands"))
        return;
    for (const auto& envelope : in["commands"])
    {
        std::string type = envelope.value("type", "");
        auto cmd = EditorCommandRegistry::Get().Create(type);
        if (cmd)
        {
            cmd->Deserialize(envelope["data"]);
            m_commands.push_back(std::move(cmd));
        }
        else
            DLOG(LogEditorCommand, ELogLevel::Error, "[Command Batch] Deserialize: Unknown command type: {}", type);
    }
}
