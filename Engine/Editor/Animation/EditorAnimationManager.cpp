// Engine/Editor/Animation/EditorAnimationManager.cpp
#include "Editor/Animation/EditorAnimationManager.h"

#include "Editor/EditorCore.h"
#include "Editor/Commands/EditorCommand_SetProperty.h"
#include "Editor/Commands/EditorCommandContext.h"

#include <algorithm>

using namespace DeltaEngine;

void EditorAnimationManager::StartAnimation(
    AssetId assetId, ObjectId objectId, std::string propertyName,
    float currentValue, float targetValue, float duration,
    std::function<void(float)> setter)
{
    EditorAnimationInstance* existing = FindInstance(assetId, objectId, propertyName);
    if (existing)
    {
        // Preempt: start new segment from current interpolated value,
        // but keep the original m_undoValue so Undo reverts all the way back.
        existing->m_fromValue   = existing->GetCurrentValue();
        existing->m_targetValue = targetValue;
        existing->m_duration    = duration;
        existing->m_elapsed     = 0.0f;
        existing->m_complete    = false;
        existing->m_setter      = std::move(setter);
    }
    else
    {
        EditorAnimationInstance inst;
        inst.m_assetId      = assetId;
        inst.m_objectId     = objectId;
        inst.m_propertyName = std::move(propertyName);
        inst.m_undoValue    = currentValue;
        inst.m_fromValue    = currentValue;
        inst.m_targetValue  = targetValue;
        inst.m_duration     = duration;
        inst.m_setter       = std::move(setter);
        m_instances.push_back(std::move(inst));
    }
}

void EditorAnimationManager::DropAnimation(
    const AssetId& assetId, const ObjectId& objectId, const std::string& propertyName)
{
    auto it = std::find_if(m_instances.begin(), m_instances.end(),
        [&](const EditorAnimationInstance& inst)
        { return inst.MatchesKey(assetId, objectId, propertyName); });

    if (it != m_instances.end())
        m_instances.erase(it);
}

bool EditorAnimationManager::CancelInFlightAnimations(EditorCore& /*core*/)
{
    if (m_instances.empty())
        return false;

    for (auto& inst : m_instances)
        inst.m_setter(inst.m_undoValue);

    m_instances.clear();
    return true;
}

bool EditorAnimationManager::HasInFlightAnimations() const
{
    return !m_instances.empty();
}

void EditorAnimationManager::Tick(float deltaTime, EditorCore& core)
{
    // Tick all instances; collect completed ones before erasing.
    std::vector<EditorAnimationInstance> completed;

    for (auto& inst : m_instances)
    {
        inst.Tick(deltaTime);
        if (inst.m_complete)
            completed.push_back(inst);
    }

    m_instances.erase(
        std::remove_if(m_instances.begin(), m_instances.end(),
            [](const EditorAnimationInstance& i) { return i.m_complete; }),
        m_instances.end());

    // Commit each completed animation as a SetProperty command so it is undoable.
    for (const auto& inst : completed)
    {
        EditorCommandContext ctx{core};
        auto cmd = std::make_unique<EditorCommand_SetProperty>(
            inst.m_assetId,
            inst.m_objectId,
            inst.m_propertyName,
            nlohmann::json(inst.m_undoValue),
            nlohmann::json(inst.m_targetValue));
        core.GetCommandManager().Execute(std::move(cmd), ctx);
    }
}

EditorAnimationInstance* EditorAnimationManager::FindInstance(
    const AssetId& assetId, const ObjectId& objectId, const std::string& propertyName)
{
    auto it = std::find_if(m_instances.begin(), m_instances.end(),
        [&](const EditorAnimationInstance& inst)
        { return inst.MatchesKey(assetId, objectId, propertyName); });

    return (it != m_instances.end()) ? &*it : nullptr;
}
