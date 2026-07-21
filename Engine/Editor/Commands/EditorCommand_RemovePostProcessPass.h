#pragma once

#include "EditorCommand.h"
#include "EditorCommandRegistry.h"

#include "Runtime/Core/UUID.h"
#include "Runtime/Serialization/ObjectSnapshot.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API EditorCommand_RemovePostProcessPass : public EditorCommand
{
public:
    EditorCommand_RemovePostProcessPass() = default;
    EditorCommand_RemovePostProcessPass(AssetId assetId, std::string className);

    static constexpr std::string_view StaticTypeName() { return "EditorCommand_RemovePostProcessPass"; }
    std::string_view GetTypeName() const override { return StaticTypeName(); }
    std::string_view GetDescription() const override;

    bool Execute(EditorCommandContext& ctx) override;
    bool Undo(EditorCommandContext& ctx) override;
    bool Redo(EditorCommandContext& ctx) override;
    void Serialize(nlohmann::json& out) const override;

private:
    AssetId m_assetId;
    std::string m_className;
    ObjectId m_removedId;
    ObjectSnapshot m_snapshot;
    int m_passIndex = -1;
    mutable std::string m_description;

    inline static CommandRegistrar<EditorCommand_RemovePostProcessPass> s_registrar{};
};

DELTA_ENGINE_NS_END
