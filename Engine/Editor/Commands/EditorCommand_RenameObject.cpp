#include "Editor/Commands/EditorCommand_RenameObject.h"
#include "Editor/Commands/PropertyValueIO.h"
#include "Editor/EditorCore.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Reflection/DClass.h"

using namespace DeltaEngine;

EditorCommand_RenameObject::EditorCommand_RenameObject(
    AssetId assetId, ObjectId targetObjectId, std::string newName)
    : m_assetId(assetId)
    , m_targetObjectId(targetObjectId)
    , m_newName(std::move(newName))
{
}

std::string_view EditorCommand_RenameObject::GetDescription() const
{
    if (m_description.empty())
        m_description = "Rename to " + m_newName;
    return m_description;
}

bool EditorCommand_RenameObject::ApplyName(EditorCommandContext& ctx, const std::string& name)
{
    DObject* obj = ctx.core.ResolveObject(m_assetId, m_targetObjectId);
    if (!obj)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Rename] Object {} not found", m_targetObjectId.ToString());
        return false;
    }

    DClass* dc = obj->GetClass();
    if (!dc)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Rename] Object {} has no class", m_targetObjectId.ToString());
        return false;
    }

    DProperty* nameProp = dc->FindPropertyByName("m_name");
    if (!nameProp)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Rename] m_name property not found on {}", dc->GetName());
        return false;
    }

    return SetPropertyFromJson(obj, nameProp, nlohmann::json(name));
}

bool EditorCommand_RenameObject::Execute(EditorCommandContext& ctx)
{
    DObject* obj = ctx.core.ResolveObject(m_assetId, m_targetObjectId);
    if (!obj)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Rename] Execute: Object {} not found, assetId {}", m_targetObjectId.ToString(), m_assetId.ToString());
        return false;
    }

    DClass* dc = obj->GetClass();
    DProperty* nameProp = dc ? dc->FindPropertyByName("m_name") : nullptr;
    if (nameProp)
    {
        const nlohmann::json jName = PropertyToJson(obj, nameProp);
        if (!jName.is_string())
        {
            DLOG(LogEditorCommand, ELogLevel::Error,
                 "[Rename] Execute: capturing undo for 'm_name' on object {} (class '{}'): "
                 "PropertyToJson must yield JSON string — got '{}' (fixes corrupt Undo without std::terminate)",
                 m_targetObjectId.ToString(),
                 dc ? dc->GetName() : "(null class)",
                 jName.type_name());
            return false;
        }
        m_oldName = jName.get<std::string>();
    }

    return ApplyName(ctx, m_newName);
}

bool EditorCommand_RenameObject::Undo(EditorCommandContext& ctx)
{
    return ApplyName(ctx, m_oldName);
}

void EditorCommand_RenameObject::Serialize(nlohmann::json& out) const
{
    out["assetId"] = m_assetId.ToString();
    out["targetObjectId"] = m_targetObjectId.ToString();
    out["newName"] = m_newName;
    out["oldName"] = m_oldName;
}

void EditorCommand_RenameObject::Deserialize(const nlohmann::json& in)
{
    m_assetId = UUID::FromString(in.value("assetId", ""));
    m_targetObjectId = UUID::FromString(in.value("targetObjectId", ""));
    m_newName = in.value("newName", "");
    m_oldName = in.value("oldName", "");
    m_description.clear();
}
