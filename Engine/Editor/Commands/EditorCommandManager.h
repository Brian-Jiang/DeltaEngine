#pragma once

#include "EditorCommand.h"
#include "EditorAuxiliaryCommand.h"

#include <memory>
#include <string_view>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class EditorCommandManager
{
public:
    DELTAEDITOR_API bool Execute(std::unique_ptr<EditorCommand> cmd, EditorCommandContext& ctx);
    DELTAEDITOR_API void ExecuteAuxiliary(std::unique_ptr<EditorAuxiliaryCommand> cmd, EditorCommandContext& ctx);
    DELTAEDITOR_API bool Undo(EditorCommandContext& ctx);
    DELTAEDITOR_API bool Redo(EditorCommandContext& ctx);

    DELTAEDITOR_API bool CanUndo() const;
    DELTAEDITOR_API bool CanRedo() const;
    DELTAEDITOR_API std::string_view GetUndoDescription() const;
    DELTAEDITOR_API std::string_view GetRedoDescription() const;

    DELTAEDITOR_API void SerializeUndoStack(nlohmann::json& out) const;
    DELTAEDITOR_API void DeserializeAndReplay(const nlohmann::json& in, EditorCommandContext& ctx);

    DELTAEDITOR_API void Clear();

    DELTAEDITOR_API size_t GetUndoStackDepth() const;
    DELTAEDITOR_API const std::vector<std::unique_ptr<EditorCommand>>& GetUndoStack() const { return m_undoStack; }
    DELTAEDITOR_API const std::vector<std::unique_ptr<EditorCommand>>& GetRedoStack() const { return m_redoStack; }

private:
    std::vector<std::unique_ptr<EditorCommand>> m_undoStack;
    std::vector<std::unique_ptr<EditorCommand>> m_redoStack;
    static constexpr int k_maxUndoDepth = 100;
};

DELTA_ENGINE_NS_END
