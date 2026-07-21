#pragma once

#include "EditorCommand.h"
#include "EditorCommandRegistry.h"

#include "Runtime/Core/UUID.h"
#include "Runtime/Serialization/ObjectSnapshot.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API EditorCommand_AddPostProcessPass : public EditorCommand
{
public:
    EditorCommand_AddPostProcessPass() = default;
    EditorCommand_AddPostProcessPass(AssetId assetId, std::string className);

    static constexpr std::string_view StaticTypeName() { return "EditorCommand_AddPostProcessPass"; }
    std::string_view GetTypeName() const override { return StaticTypeName(); }
    std::string_view GetDescription() const override;

    bool Execute(EditorCommandContext& ctx) override;
    bool Undo(EditorCommandContext& ctx) override;
    bool Redo(EditorCommandContext& ctx) override;
    void Serialize(nlohmann::json& out) const override;

    /** Object id of the pass created by Execute (null before Execute). */
    ObjectId GetCreatedObjectId() const { return m_createdId; }

    /** Index the pass was inserted at (-1 before Execute). */
    int GetPassIndex() const { return m_passIndex; }

private:
    AssetId m_assetId;
    std::string m_className;
    ObjectId m_createdId;
    ObjectSnapshot m_snapshot;
    int m_passIndex = -1;
    bool m_hasSnapshot = false;
    mutable std::string m_description;

    inline static CommandRegistrar<EditorCommand_AddPostProcessPass> s_registrar{};
};

DELTA_ENGINE_NS_END
