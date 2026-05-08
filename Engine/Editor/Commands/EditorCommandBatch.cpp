#include "Editor/Commands/EditorCommandBatch.h"

using namespace DeltaEngine;

EditorCommandBatch::EditorCommandBatch(std::string description)
    : m_description(std::move(description))
{
}

void EditorCommandBatch::Add(std::unique_ptr<EditorCommand> cmd)
{
    DELTA_ASSERT_MSG(cmd != nullptr, "EditorCommandBatch::Add rejected null command");
    m_commands.push_back(std::move(cmd));
}

bool EditorCommandBatch::Execute(EditorCommandContext& ctx)
{
    if (m_commands.empty())
    {
        DLOG(LogEditorCommand, ELogLevel::Warning,
             "[Command Batch] Execute: empty batch (expected at least one sub-command)");
        return false;
    }

    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Command Batch] Execute: Start ({} commands)", m_commands.size());
    for (auto& cmd : m_commands)
    {
        if (!cmd->Execute(ctx))
        {
            DLOG(LogEditorCommand, ELogLevel::Error,
                 "[Command Batch] Execute: sub-command '{}' failed — prior sub-commands applied; Undo whole batch individually if needed",
                 cmd->GetTypeName());
            return false;
        }
    }
    return true;
}

bool EditorCommandBatch::Undo(EditorCommandContext& ctx)
{
    if (m_commands.empty())
    {
        DLOG(LogEditorCommand, ELogLevel::Warning, "[Command Batch] Undo: empty batch");
        return false;
    }

    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Command Batch] Undo: Start ({} commands)", m_commands.size());
    for (auto it = m_commands.rbegin(); it != m_commands.rend(); ++it)
    {
        if (!(*it)->Undo(ctx))
        {
            DLOG(LogEditorCommand, ELogLevel::Error, "[Command Batch] Undo: sub-command '{}' failed", (*it)->GetTypeName());
            return false;
        }
    }
    return true;
}

bool EditorCommandBatch::Redo(EditorCommandContext& ctx)
{
    if (m_commands.empty())
    {
        DLOG(LogEditorCommand, ELogLevel::Warning, "[Command Batch] Redo: empty batch");
        return false;
    }

    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Command Batch] Redo: Start ({} commands)", m_commands.size());
    for (auto& cmd : m_commands)
    {
        if (!cmd->Redo(ctx))
        {
            DLOG(LogEditorCommand, ELogLevel::Error, "[Command Batch] Redo: sub-command '{}' failed", cmd->GetTypeName());
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
        if (!cmd)
        {
            if (!type.empty())
                DLOG(LogEditorCommand, ELogLevel::Error,
                     "[Command Batch] Deserialize: unknown sub-command type '{}' — entry skipped "
                     "(batch may mismatch serialized editor version)",
                     type);
            continue;
        }
        if (!envelope.contains("data"))
        {
            DLOG(LogEditorCommand, ELogLevel::Error,
                 "[Command Batch] Deserialize: sub-command '{}' missing 'data'",
                 type);
            continue;
        }
        try
        {
            cmd->Deserialize(envelope["data"]);
        }
        catch (const std::exception& e)
        {
            DLOG(LogEditorCommand, ELogLevel::Error,
                 "[Command Batch] Deserialize failed for '{}': {}",
                 type, e.what());
            continue;
        }
        m_commands.push_back(std::move(cmd));
    }
}
