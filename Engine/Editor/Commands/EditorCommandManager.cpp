#include "Editor/Commands/EditorCommandManager.h"

#include "Editor/EditorCore.h"
#include "Editor/Animation/EditorAnimationManager.h"

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
    // If there are in-flight animations, cancel them and revert their values.
    // This counts as consuming the Undo action — the stack is NOT popped, so a
    // subsequent Undo will pop the last committed command as normal.
    if (auto* animMgr = ctx.core.GetAnimationManager())
    {
        if (animMgr->CancelInFlightAnimations(ctx.core))
            return true;
    }

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

void EditorCommandManager::Clear()
{
    m_undoStack.clear();
    m_redoStack.clear();
}

size_t EditorCommandManager::GetUndoStackDepth() const
{
    return m_undoStack.size();
}
