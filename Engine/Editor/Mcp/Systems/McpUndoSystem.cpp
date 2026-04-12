#include "McpUndoSystem.h"

#include "EditorCore.h"
#include "Commands/EditorCommandManager.h"
#include "Mcp/McpRegistry.h"

using namespace DeltaEngine;

void McpUndoSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterOperation("undo_history", "stack",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryStack(c, p); });
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
