#pragma once

#include "EditorCommand.h"
#include "EditorCommandRegistry.h"

#include "Runtime/Core/UUID.h"
#include "Runtime/Serialization/ObjectSnapshot.h"

#include <array>
#include <optional>
#include <string>

DELTA_ENGINE_NS_BEGIN

/** Duplicates a GameObject and its full component subtree under fresh object ids. */
class DELTAEDITOR_API EditorCommand_DuplicateGameObject : public EditorCommand
{
public:
    EditorCommand_DuplicateGameObject() = default;
    EditorCommand_DuplicateGameObject(AssetId assetId, ObjectId sourceGameObjectId,
                                      std::string newName = {},
                                      std::optional<std::array<float, 3>> offsetPosition = std::nullopt);

    static constexpr std::string_view StaticTypeName() { return "EditorCommand_DuplicateGameObject"; }
    std::string_view GetTypeName() const override { return StaticTypeName(); }
    std::string_view GetDescription() const override;

    bool Execute(EditorCommandContext& ctx) override;
    bool Undo(EditorCommandContext& ctx) override;
    bool Redo(EditorCommandContext& ctx) override;
    void Serialize(nlohmann::json& out) const override;
    void Deserialize(const nlohmann::json& in) override;

    /** Object id of the duplicated GameObject created by Execute (null before Execute). */
    ObjectId GetCreatedObjectId() const { return m_createdId; }

private:
    bool RestoreDuplicate(EditorCommandContext& ctx);

    AssetId m_assetId;
    ObjectId m_sourceGameObjectId;
    std::string m_newName;
    bool m_hasOffset = false;
    float m_offset[3] = {0.0f, 0.0f, 0.0f};

    ObjectId m_createdId;
    ObjectSnapshot m_snapshot;
    bool m_hasSnapshot = false;
    mutable std::string m_description;

    inline static CommandRegistrar<EditorCommand_DuplicateGameObject> s_registrar{};
};

DELTA_ENGINE_NS_END
