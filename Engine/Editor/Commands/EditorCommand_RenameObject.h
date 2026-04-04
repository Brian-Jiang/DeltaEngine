#pragma once

#include "EditorCommand.h"
#include "EditorCommandRegistry.h"

#include "Runtime/Core/UUID.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

class EditorCommand_RenameObject : public EditorCommand
{
public:
    EditorCommand_RenameObject() = default;
    EditorCommand_RenameObject(AssetId assetId, ObjectId targetObjectId, std::string newName);

    static constexpr std::string_view StaticTypeName() { return "EditorCommand_RenameObject"; }
    std::string_view GetTypeName() const override { return StaticTypeName(); }
    std::string_view GetDescription() const override;

    bool Execute(EditorCommandContext& ctx) override;
    bool Undo(EditorCommandContext& ctx) override;
    void Serialize(nlohmann::json& out) const override;
    void Deserialize(const nlohmann::json& in) override;

private:
    bool ApplyName(EditorCommandContext& ctx, const std::string& name);

    AssetId     m_assetId;
    ObjectId    m_targetObjectId;
    std::string m_newName;
    std::string m_oldName;
    mutable std::string m_description;

    inline static CommandRegistrar<EditorCommand_RenameObject> s_registrar{};
};

DELTA_ENGINE_NS_END
