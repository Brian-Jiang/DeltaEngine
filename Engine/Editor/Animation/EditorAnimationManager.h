// Engine/Editor/Animation/EditorAnimationManager.h
#pragma once

#include "EditorIncludes.h"

#include "Runtime/Core/UUID.h"
#include "Editor/Animation/EditorAnimationInstance.h"

#include <functional>
#include <string>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class EditorCore;

class EditorAnimationManager
{
public:
    // Start a new animation for (assetId, objectId, propertyName).
    // If an animation for the same key is already running: preempts it — the new animation
    // starts from the current interpolated value, but m_undoValue is preserved from the
    // original animation so that Undo still reverts to the pre-animation state.
    // If no existing animation: m_undoValue = currentValue.
    DELTAEDITOR_API void StartAnimation(
        AssetId assetId, ObjectId objectId, std::string propertyName,
        float currentValue, float targetValue, float duration,
        std::function<void(float)> setter);

    // Silently remove any in-flight animation for the given key WITHOUT reverting the value.
    // Called by EditorCommand_SetProperty::Execute so a direct property write wins over an animation.
    DELTAEDITOR_API void DropAnimation(
        const AssetId& assetId, const ObjectId& objectId, const std::string& propertyName);

    // Cancel ALL in-flight animations: revert each to its m_undoValue via its setter.
    // Returns true if at least one animation was cancelled.
    // Called by EditorCommandManager::Undo so the user can undo an in-flight animation
    // without popping the undo stack.
    DELTAEDITOR_API bool CancelInFlightAnimations(EditorCore& core);

    DELTAEDITOR_API bool HasInFlightAnimations() const;
    DELTAEDITOR_API size_t GetInstanceCount() const { return m_instances.size(); }

    // Advance all animations by deltaTime. Completed animations are committed to the
    // undo stack as EditorCommand_SetProperty commands via core.GetCommandManager().
    DELTAEDITOR_API void Tick(float deltaTime, EditorCore& core);

private:
    EditorAnimationInstance* FindInstance(
        const AssetId& assetId, const ObjectId& objectId, const std::string& propertyName);

    std::vector<EditorAnimationInstance> m_instances;
};

DELTA_ENGINE_NS_END
