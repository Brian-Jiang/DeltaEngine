#pragma once

#include "EditorCommand.h"
#include "EditorCommandRegistry.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API EditorCommand_SetTestValue : public EditorCommand
{
public:
    EditorCommand_SetTestValue() = default;
    EditorCommand_SetTestValue(std::string key, std::string valueBefore, std::string valueAfter);

    static constexpr std::string_view StaticTypeName() { return "EditorCommand_SetTestValue"; }
    std::string_view GetTypeName() const override { return StaticTypeName(); }

    bool Execute(EditorCommandContext& ctx) override;
    bool Undo(EditorCommandContext& ctx) override;
    void Serialize(nlohmann::json& out) const override;
    void Deserialize(const nlohmann::json& in) override;

private:
    std::string m_key;
    std::string m_valueBefore;
    std::string m_valueAfter;

    inline static CommandRegistrar<EditorCommand_SetTestValue> s_registrar{};
};

DELTA_ENGINE_NS_END
