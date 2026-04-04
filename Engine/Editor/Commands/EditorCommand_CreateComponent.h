#pragma once

#include "EditorCommand.h"
#include "EditorCommandRegistry.h"

#include "Runtime/Core/UUID.h"
#include "Runtime/Serialization/ObjectSnapshot.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API EditorCommand_CreateComponent : public EditorCommand
{
public:
    EditorCommand_CreateComponent() = default;
    EditorCommand_CreateComponent(AssetId sceneAssetId, ObjectId gameObjectId, std::string componentClassName);

    static constexpr std::string_view StaticTypeName() { return "EditorCommand_CreateComponent"; }
    std::string_view GetTypeName() const override { return StaticTypeName(); }
    std::string_view GetDescription() const override;

    bool Execute(EditorCommandContext& ctx) override;
    bool Undo(EditorCommandContext& ctx) override;
    bool Redo(EditorCommandContext& ctx) override;
    void Serialize(nlohmann::json& out) const override;
    void Deserialize(const nlohmann::json& in) override;

private:
    AssetId m_sceneAssetId;
    ObjectId m_gameObjectId;
    std::string m_className;
    ObjectId m_createdComponentId;
    ObjectSnapshot m_snapshot;
    bool m_isSceneComponent = false;
    int m_componentIndex = -1;
    ObjectId m_parentSceneComponentId;
    bool m_hasSnapshot = false;
    mutable std::string m_description;

    inline static CommandRegistrar<EditorCommand_CreateComponent> s_registrar{};
};

DELTA_ENGINE_NS_END
