#pragma once

#include "EditorCommand.h"
#include "EditorCommandRegistry.h"

#include "Runtime/Core/UUID.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

class EditorCommand_SetProperty : public EditorCommand
{
public:
    EditorCommand_SetProperty() = default;
    EditorCommand_SetProperty(AssetId assetId, ObjectId objectId,
                              std::string propertyName,
                              nlohmann::json valueBefore,
                              nlohmann::json valueAfter);

    static constexpr std::string_view StaticTypeName() { return "EditorCommand_SetProperty"; }
    std::string_view GetTypeName() const override { return StaticTypeName(); }
    std::string_view GetDescription() const override;

    bool Execute(EditorCommandContext& ctx) override;
    bool Undo(EditorCommandContext& ctx) override;
    void Serialize(nlohmann::json& out) const override;
    void Deserialize(const nlohmann::json& in) override;

private:
    bool ApplyValue(EditorCommandContext& ctx, const nlohmann::json& value);

    AssetId        m_assetId;
    ObjectId       m_objectId;
    std::string    m_propertyName;
    nlohmann::json m_valueBefore;
    nlohmann::json m_valueAfter;
    mutable std::string m_description;

    inline static CommandRegistrar<EditorCommand_SetProperty> s_registrar{};
};

DELTA_ENGINE_NS_END
