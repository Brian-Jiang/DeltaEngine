#pragma once

#include "EditorCommand.h"
#include "EditorCommandRegistry.h"

#include <memory>
#include <string>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class EditorCommandBatch : public EditorCommand
{
public:
    EditorCommandBatch() = default;
    explicit EditorCommandBatch(std::string description);

    void Add(std::unique_ptr<EditorCommand> cmd);

    static constexpr std::string_view StaticTypeName() { return "EditorCommandBatch"; }
    std::string_view GetTypeName() const override { return StaticTypeName(); }
    std::string_view GetDescription() const override { return m_description; }

    bool Execute(EditorCommandContext& ctx) override;
    bool Undo(EditorCommandContext& ctx) override;
    bool Redo(EditorCommandContext& ctx) override;
    void Serialize(nlohmann::json& out) const override;
    void Deserialize(const nlohmann::json& in) override;

private:
    std::vector<std::unique_ptr<EditorCommand>> m_commands;
    std::string m_description;

    inline static CommandRegistrar<EditorCommandBatch> s_registrar{};
};

DELTA_ENGINE_NS_END
