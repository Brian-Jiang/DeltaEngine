#include "EditorCommand_SetTestValue.h"

#include "Editor/EditorCore.h"

// PATTERN FOR OBJECT-TARGETING COMMANDS:
// Always resolve the target object fresh inside Execute/Undo/Redo:
//   auto* obj = ctx.core.ResolveObject<MyType>(m_assetId, m_objectId);
//   if (!obj) return false;
// Never store resolved pointers as members — they are invalid across undo/redo.

using namespace DeltaEngine;

EditorCommand_SetTestValue::EditorCommand_SetTestValue(std::string key, std::string valueBefore, std::string valueAfter)
    : m_key(std::move(key))
    , m_valueBefore(std::move(valueBefore))
    , m_valueAfter(std::move(valueAfter))
{
}

bool EditorCommand_SetTestValue::Execute(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Set Test Value] Execute: Start (key='{}')", m_key);
    ctx.core.SetTestValue(m_key, m_valueAfter);
    return true;
}

bool EditorCommand_SetTestValue::Undo(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Set Test Value] Undo: Start (key='{}')", m_key);
    ctx.core.SetTestValue(m_key, m_valueBefore);
    return true;
}

void EditorCommand_SetTestValue::Serialize(nlohmann::json& out) const
{
    out["key"] = m_key;
    out["valueBefore"] = m_valueBefore;
    out["valueAfter"] = m_valueAfter;
}

void EditorCommand_SetTestValue::Deserialize(const nlohmann::json& in)
{
    m_key = in.value("key", "");
    m_valueBefore = in.value("valueBefore", "");
    m_valueAfter = in.value("valueAfter", "");
}
