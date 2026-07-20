#pragma once

#include "EditorCommand.h"
#include "EditorCommandRegistry.h"

#include "Runtime/Core/UUID.h"
#include "Runtime/Serialization/ObjectSnapshot.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API EditorCommand_CreateGameObject : public EditorCommand
{
public:
    EditorCommand_CreateGameObject() = default;
    EditorCommand_CreateGameObject(AssetId sceneAssetId, std::string className, std::string initialName = {});

    static constexpr std::string_view StaticTypeName() { return "EditorCommand_CreateGameObject"; }
    std::string_view GetTypeName() const override { return StaticTypeName(); }
    std::string_view GetDescription() const override;

    bool Execute(EditorCommandContext& ctx) override;
    bool Undo(EditorCommandContext& ctx) override;
    bool Redo(EditorCommandContext& ctx) override;
    void Serialize(nlohmann::json& out) const override;
    void Deserialize(const nlohmann::json& in) override;

    /** Object id of the GameObject created by Execute (null before Execute). */
    ObjectId GetCreatedObjectId() const { return m_createdId; }

private:
    AssetId m_sceneAssetId;
    std::string m_className;
    std::string m_initialName;
    ObjectId m_createdId;
    ObjectSnapshot m_snapshot;
    mutable std::string m_description;
    bool m_hasSnapshot = false;

    inline static CommandRegistrar<EditorCommand_CreateGameObject> s_registrar{};
};

DELTA_ENGINE_NS_END
