#pragma once

#include "EditorCommand.h"
#include "EditorCommandRegistry.h"

#include "Runtime/Core/UUID.h"
#include "Runtime/Serialization/ObjectSnapshot.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API EditorCommand_DeleteGameObject : public EditorCommand
{
public:
    EditorCommand_DeleteGameObject() = default;
    EditorCommand_DeleteGameObject(AssetId assetId, ObjectId gameObjectId);

    static constexpr std::string_view StaticTypeName() { return "EditorCommand_DeleteGameObject"; }
    std::string_view GetTypeName() const override { return StaticTypeName(); }
    std::string_view GetDescription() const override;

    bool Execute(EditorCommandContext& ctx) override;
    bool Undo(EditorCommandContext& ctx) override;
    void Serialize(nlohmann::json& out) const override;
    void Deserialize(const nlohmann::json& in) override;

private:
    AssetId m_assetId;
    ObjectId m_gameObjectId;
    ObjectSnapshot m_snapshot;
    mutable std::string m_description;

    inline static CommandRegistrar<EditorCommand_DeleteGameObject> s_registrar{};
};

DELTA_ENGINE_NS_END
