#include "Editor/Commands/EditorCommand_SetProperty.h"
#include "Editor/Commands/PropertyValueIO.h"
#include "Editor/EditorCore.h"

#include "Runtime/Reflection/DClass.h"
#include "Runtime/Core/DObject.h"

using namespace DeltaEngine;

EditorCommand_SetProperty::EditorCommand_SetProperty(
    AssetId assetId, ObjectId objectId,
    std::string propertyName,
    nlohmann::json valueBefore,
    nlohmann::json valueAfter)
    : m_assetId(assetId)
    , m_objectId(objectId)
    , m_propertyName(std::move(propertyName))
    , m_valueBefore(std::move(valueBefore))
    , m_valueAfter(std::move(valueAfter))
{
}

std::string_view EditorCommand_SetProperty::GetDescription() const
{
    if (m_description.empty())
        m_description = "Set " + m_propertyName;
    return m_description;
}

bool EditorCommand_SetProperty::ApplyValue(EditorCommandContext& ctx, const nlohmann::json& value)
{
    DObject* obj = ctx.core.ResolveObject(m_assetId, m_objectId);
    if (!obj)
        return false;

    DClass* dc = obj->GetClass();
    if (!dc)
        return false;

    DProperty* prop = dc->FindPropertyByName(m_propertyName);
    if (!prop)
        return false;

    return SetPropertyFromJson(obj, prop, value);
}

bool EditorCommand_SetProperty::Execute(EditorCommandContext& ctx)
{
    return ApplyValue(ctx, m_valueAfter);
}

bool EditorCommand_SetProperty::Undo(EditorCommandContext& ctx)
{
    return ApplyValue(ctx, m_valueBefore);
}

void EditorCommand_SetProperty::Serialize(nlohmann::json& out) const
{
    out["assetId"] = m_assetId.ToString();
    out["objectId"] = m_objectId.ToString();
    out["propertyName"] = m_propertyName;
    out["valueBefore"] = m_valueBefore;
    out["valueAfter"] = m_valueAfter;
}

void EditorCommand_SetProperty::Deserialize(const nlohmann::json& in)
{
    m_assetId = UUID::FromString(in.value("assetId", ""));
    m_objectId = UUID::FromString(in.value("objectId", ""));
    m_propertyName = in.value("propertyName", "");
    m_valueBefore = in.value("valueBefore", nlohmann::json{});
    m_valueAfter = in.value("valueAfter", nlohmann::json{});
    m_description.clear();
}
