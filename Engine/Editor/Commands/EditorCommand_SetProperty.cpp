#include "Editor/Commands/EditorCommand_SetProperty.h"
#include "Editor/Commands/PropertyValueIO.h"
#include "Editor/EditorCore.h"
#include "Editor/Animation/EditorAnimationManager.h"

#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
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
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Set Property] Object {} not found in asset {}", m_objectId.ToString(), m_assetId.ToString());
        return false;
    }

    DClass* dc = obj->GetClass();
    if (!dc)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Set Property] Object {} has no class", m_objectId.ToString());
        return false;
    }

    DProperty* prop = dc->FindPropertyByName(m_propertyName);
    if (!prop)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Set Property] Property '{}' not found on object {}", m_propertyName, m_objectId.ToString());
        return false;
    }

    const bool ok = (prop->GetPropertyType() == EPropertyType::ObjectPtr ||
                     prop->GetPropertyType() == EPropertyType::Vector)
        ? SetPropertyFromJson(obj, prop, value, ctx.core)
        : SetPropertyFromJson(obj, prop, value);

    if (!ok)
    {
        DLOG(LogEditorCommand, ELogLevel::Error, "[Set Property] SetPropertyFromJson failed for '{}' on object {}", m_propertyName, m_objectId.ToString());
        return false;
    }
    return true;
}

bool EditorCommand_SetProperty::Execute(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Set Property] Execute: Start");

    // Drop any in-flight animation targeting this property. The incoming SetProperty
    // value wins; the animation is silently discarded (no revert, no undo entry).
    if (auto* animMgr = ctx.core.GetAnimationManager())
    {
        animMgr->DropAnimation(m_assetId, m_objectId, m_propertyName);
        // A direct write to m_localTransform also wins over all transform channels.
        if (m_propertyName == "m_localTransform")
            animMgr->DropTransformAnimations(m_assetId, m_objectId);
    }

    if (m_valueBefore.is_null())
    {
        DObject* obj = ctx.core.ResolveObject(m_assetId, m_objectId);
        if (obj)
        {
            DClass* dc = obj->GetClass();
            DProperty* prop = dc ? dc->FindPropertyByName(m_propertyName) : nullptr;
            if (prop)
                m_valueBefore = PropertyToJson(obj, prop);
        }
    }

    return ApplyValue(ctx, m_valueAfter);
}

bool EditorCommand_SetProperty::Undo(EditorCommandContext& ctx)
{
    DLOG(LogEditorCommand, ELogLevel::Verbose, "[Set Property] Undo: Start");
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
