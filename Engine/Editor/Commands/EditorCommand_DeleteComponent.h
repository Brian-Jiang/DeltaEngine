#pragma once

#include "EditorCommand.h"
#include "EditorCommandRegistry.h"

#include "Runtime/Core/UUID.h"
#include "Runtime/Serialization/ObjectSnapshot.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API EditorCommand_DeleteComponent : public EditorCommand
{
public:
    EditorCommand_DeleteComponent() = default;
    EditorCommand_DeleteComponent(AssetId sceneAssetId, ObjectId gameObjectId, ObjectId componentId);

    static constexpr std::string_view StaticTypeName() { return "EditorCommand_DeleteComponent"; }
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
    ObjectId m_componentId;
    ObjectSnapshot m_snapshot;
    bool m_isSceneComponent = false;
    int m_componentIndex = -1;
    ObjectId m_parentSceneComponentId;
    mutable std::string m_description;

    inline static CommandRegistrar<EditorCommand_DeleteComponent> s_registrar{};
};

DELTA_ENGINE_NS_END
