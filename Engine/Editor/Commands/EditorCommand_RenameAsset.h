#pragma once

#include "EditorCommand.h"
#include "EditorCommandRegistry.h"

#include "Runtime/Core/UUID.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API EditorCommand_RenameAsset : public EditorCommand
{
public:
    EditorCommand_RenameAsset() = default;
    EditorCommand_RenameAsset(AssetId assetId, std::string desiredStem);

    static constexpr std::string_view StaticTypeName() { return "EditorCommand_RenameAsset"; }
    std::string_view GetTypeName() const override { return StaticTypeName(); }
    std::string_view GetDescription() const override;

    bool Execute(EditorCommandContext& ctx) override;
    bool Undo(EditorCommandContext& ctx) override;
    void Serialize(nlohmann::json& out) const override;

private:
    AssetId     m_assetId;
    std::string m_desiredStem;
    std::string m_oldStem;
    std::string m_newStem;
    mutable std::string m_description;

    inline static CommandRegistrar<EditorCommand_RenameAsset> s_registrar{};
};

DELTA_ENGINE_NS_END
