#include "McpUndoSystem.h"

#include "EditorCore.h"
#include "Commands/EditorCommandContext.h"
#include "Commands/EditorCommandManager.h"
#include "Mcp/McpProtocol.h"
#include "Mcp/McpRegistry.h"

using namespace DeltaEngine;

void McpUndoSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterQuery("undo_history", "stack",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryStack(c, p); });
    registry.RegisterCommand("undo_history", "Undo",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandUndo(c, p); });
    registry.RegisterCommand("undo_history", "Redo",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandRedo(c, p); });
}

static nlohmann::json SerializeEntry(const EditorCommand& cmd, bool includeData)
{
    nlohmann::json entry;
    entry["type"] = cmd.GetTypeName();
    entry["description"] = cmd.GetDescription();
    if (includeData)
        cmd.Serialize(entry["data"]);
    return entry;
}

nlohmann::json McpUndoSystem::QueryStack(EditorCore& core, const nlohmann::json& params)
{
    int maxEntries = params.value("max_entries", 50);
    bool includeData = params.value("include_data", false);

    auto& mgr = core.GetCommandManager();
    const auto& undoStack = mgr.GetUndoStack();
    const auto& redoStack = mgr.GetRedoStack();

    nlohmann::json undoArr = nlohmann::json::array();
    int undoStart = std::max(0, static_cast<int>(undoStack.size()) - maxEntries);
    for (int i = static_cast<int>(undoStack.size()) - 1; i >= undoStart; --i)
        undoArr.push_back(SerializeEntry(*undoStack[i], includeData));

    nlohmann::json redoArr = nlohmann::json::array();
    int redoCount = std::min(maxEntries, static_cast<int>(redoStack.size()));
    for (int i = static_cast<int>(redoStack.size()) - 1; i >= static_cast<int>(redoStack.size()) - redoCount; --i)
        redoArr.push_back(SerializeEntry(*redoStack[i], includeData));

    return {
        {"ok", true},
        {"undo_stack", std::move(undoArr)},
        {"redo_stack", std::move(redoArr)},
        {"can_undo", mgr.CanUndo()},
        {"can_redo", mgr.CanRedo()}
    };
}

nlohmann::json McpUndoSystem::CommandUndo(EditorCore& core, const nlohmann::json& params)
{
    int steps = params.value("steps", 1);
    if (steps < 1)
        return MakeMcpError("steps must be >= 1");

    auto& mgr = core.GetCommandManager();
    if (!mgr.CanUndo())
    {
        return {
            {"ok", false},
            {"error", "nothing to undo"},
            {"can_undo", false},
            {"can_redo", mgr.CanRedo()},
            {"expects_result", false}
        };
    }

    EditorCommandContext ctx{core};
    int done = 0;
    for (; done < steps && mgr.CanUndo(); ++done)
    {
        if (!mgr.Undo(ctx))
            return {
                {"ok", false},
                {"error", "Undo failed"},
                {"steps_done", done},
                {"can_undo", mgr.CanUndo()},
                {"can_redo", mgr.CanRedo()},
                {"expects_result", false}
            };
    }

    return {
        {"ok", true},
        {"steps_done", done},
        {"can_undo", mgr.CanUndo()},
        {"can_redo", mgr.CanRedo()},
        {"expects_result", false}
    };
}

nlohmann::json McpUndoSystem::CommandRedo(EditorCore& core, const nlohmann::json& params)
{
    int steps = params.value("steps", 1);
    if (steps < 1)
        return MakeMcpError("steps must be >= 1");

    auto& mgr = core.GetCommandManager();
    if (!mgr.CanRedo())
        return {
            {"ok", false},
            {"error", "nothing to redo"},
            {"can_undo", mgr.CanUndo()},
            {"can_redo", false},
            {"expects_result", false}
        };

    EditorCommandContext ctx{core};
    int done = 0;
    for (; done < steps && mgr.CanRedo(); ++done)
    {
        if (!mgr.Redo(ctx))
            return {
                {"ok", false},
                {"error", "Redo failed"},
                {"steps_done", done},
                {"can_undo", mgr.CanUndo()},
                {"can_redo", mgr.CanRedo()},
                {"expects_result", false}
            };
    }

    return {
        {"ok", true},
        {"steps_done", done},
        {"can_undo", mgr.CanUndo()},
        {"can_redo", mgr.CanRedo()},
        {"expects_result", false}
    };
}
