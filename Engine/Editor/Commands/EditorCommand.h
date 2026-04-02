#pragma once

#include "EditorIncludes.h"

#include "EditorCommandContext.h"

#include <nlohmann/json.hpp>
#include <string_view>

DELTA_ENGINE_NS_BEGIN

DECLARE_LOG_CATEGORY(LogEditorCommand)

class EditorCommand
{
public:
    virtual ~EditorCommand() = default;

    virtual std::string_view GetTypeName() const = 0;
    virtual bool Execute(EditorCommandContext& ctx) = 0;
    virtual bool Undo(EditorCommandContext& ctx) = 0;
    virtual bool Redo(EditorCommandContext& ctx) { return Execute(ctx); }
    virtual void Serialize(nlohmann::json& out) const = 0;
    virtual void Deserialize(const nlohmann::json& in) = 0;
    virtual std::string_view GetDescription() const { return GetTypeName(); }
};

DELTA_ENGINE_NS_END
