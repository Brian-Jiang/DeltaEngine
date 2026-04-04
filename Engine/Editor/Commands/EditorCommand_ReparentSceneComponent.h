#pragma once

#include "EditorCommand.h"
#include "EditorCommandRegistry.h"

#include "Runtime/Core/UUID.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

class EditorCommand_ReparentSceneComponent : public EditorCommand
{
public:
    EditorCommand_ReparentSceneComponent() = default;
    EditorCommand_ReparentSceneComponent(AssetId sceneAssetId, ObjectId childObjectId, ObjectId newParentObjectId);

    static constexpr std::string_view StaticTypeName() { return "EditorCommand_ReparentSceneComponent"; }
    std::string_view GetTypeName() const override { return StaticTypeName(); }
    std::string_view GetDescription() const override;

    bool Execute(EditorCommandContext& ctx) override;
    bool Undo(EditorCommandContext& ctx) override;
    void Serialize(nlohmann::json& out) const override;
    void Deserialize(const nlohmann::json& in) override;

private:
    bool ApplyReparent(EditorCommandContext& ctx, const ObjectId& newParentId);

    AssetId  m_sceneAssetId;
    ObjectId m_childObjectId;
    ObjectId m_newParentObjectId;
    ObjectId m_oldParentObjectId;
    mutable std::string m_description;

    inline static CommandRegistrar<EditorCommand_ReparentSceneComponent> s_registrar{};
};

DELTA_ENGINE_NS_END
