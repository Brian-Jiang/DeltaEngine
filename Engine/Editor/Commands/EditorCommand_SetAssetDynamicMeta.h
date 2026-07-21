#pragma once

#include "EditorCommand.h"
#include "EditorCommandRegistry.h"

#include "Runtime/Core/UUID.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API EditorCommand_SetAssetDynamicMeta : public EditorCommand
{
public:
    EditorCommand_SetAssetDynamicMeta() = default;
    EditorCommand_SetAssetDynamicMeta(AssetId assetId,
                                      std::string jsonPointer,
                                      nlohmann::json newValue);

    static constexpr std::string_view StaticTypeName() { return "EditorCommand_SetAssetDynamicMeta"; }
    std::string_view GetTypeName() const override { return StaticTypeName(); }
    std::string_view GetDescription() const override;

    bool Execute(EditorCommandContext& ctx) override;
    bool Undo(EditorCommandContext& ctx) override;
    void Serialize(nlohmann::json& out) const override;

private:
    bool ApplyMutation(EditorCommandContext& ctx,
                       const nlohmann::json& value,
                       bool valuePresent);

    AssetId        m_assetId;
    std::string    m_jsonPointer;
    nlohmann::json m_valueBefore;
    nlohmann::json m_valueAfter;
    bool           m_hadValueBefore = false;
    bool           m_snapshotTaken  = false;
    mutable std::string m_description;

    inline static CommandRegistrar<EditorCommand_SetAssetDynamicMeta> s_registrar{};
};

DELTA_ENGINE_NS_END
