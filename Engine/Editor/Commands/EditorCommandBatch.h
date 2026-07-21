#pragma once

#include "EditorCommand.h"
#include "EditorCommandRegistry.h"

#include <memory>
#include <string>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API EditorCommandBatch : public EditorCommand
{
public:
    EditorCommandBatch() = default;
    explicit EditorCommandBatch(std::string description);
    EditorCommandBatch(const EditorCommandBatch&)            = delete;
    EditorCommandBatch& operator=(const EditorCommandBatch&) = delete;
    EditorCommandBatch(EditorCommandBatch&&)                 = default;
    EditorCommandBatch& operator=(EditorCommandBatch&&)      = default;

    void Add(std::unique_ptr<EditorCommand> cmd);

    static constexpr std::string_view StaticTypeName() { return "EditorCommandBatch"; }
    std::string_view GetTypeName() const override { return StaticTypeName(); }
    std::string_view GetDescription() const override { return m_description; }

    bool Execute(EditorCommandContext& ctx) override;
    bool Undo(EditorCommandContext& ctx) override;
    bool Redo(EditorCommandContext& ctx) override;
    void Serialize(nlohmann::json& out) const override;

private:
    std::vector<std::unique_ptr<EditorCommand>> m_commands;
    std::string m_description;

    inline static CommandRegistrar<EditorCommandBatch> s_registrar{};
};

DELTA_ENGINE_NS_END
