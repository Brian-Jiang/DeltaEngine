// Engine/Editor/Animation/EditorAnimationManager.h
#pragma once

#include "EditorIncludes.h"

#include "Runtime/Core/UUID.h"
#include "Editor/Animation/EditorAnimationInstance.h"
#include "Editor/Animation/TransformAnimationTypes.h"

#include <DirectXMath.h>
#include <SimpleMath.h>
#include <nlohmann/json.hpp>

#include <functional>
#include <string>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class EditorCore;

class EditorAnimationManager
{
public:
    // Scalar (float) channel — unchanged from Phase 1.
    // Preempts any existing animation on the same key; preserves the original m_undoValue.
    DELTAEDITOR_API void StartAnimation(
        AssetId assetId, ObjectId objectId, std::string propertyName,
        float currentValue, float targetValue, float duration,
        std::function<void(float)> setter);

    // Vector3 transform channel (channel name "position" or "scale").
    // localTransformSnapshot: serialized m_localTransform at session start; ignored if a session
    // for this (assetId, objectId) already exists.
    DELTAEDITOR_API void StartAnimationVec3(
        AssetId assetId, ObjectId objectId, std::string channelName,
        DirectX::SimpleMath::Vector3 fromValue,
        DirectX::SimpleMath::Vector3 targetValue,
        float duration,
        std::function<void(DirectX::SimpleMath::Vector3)> setter,
        const nlohmann::json& localTransformSnapshot);

    // Quaternion transform channel (channel name "rotation").
    DELTAEDITOR_API void StartAnimationQuat(
        AssetId assetId, ObjectId objectId, std::string channelName,
        DirectX::SimpleMath::Quaternion fromValue,
        DirectX::SimpleMath::Quaternion targetValue,
        float duration,
        std::function<void(DirectX::SimpleMath::Quaternion)> setter,
        const nlohmann::json& localTransformSnapshot);

    // Silently remove a scalar animation for the given key WITHOUT reverting.
    // Called by EditorCommand_SetProperty::Execute so a direct property write wins.
    DELTAEDITOR_API void DropAnimation(
        const AssetId& assetId, const ObjectId& objectId, const std::string& propertyName);

    // Drop all Vec3/Quat channels and the session for (assetId, objectId) without reverting.
    // Called when EditorCommand_SetProperty writes m_localTransform directly.
    DELTAEDITOR_API void DropTransformAnimations(
        const AssetId& assetId, const ObjectId& objectId);

    // Cancel ALL in-flight animations: revert scalar channels to their undoValue and restore
    // transform sessions to their snapshots via SetPropertyFromJson. Returns true if anything
    // was cancelled. Called by EditorCommandManager::Undo.
    DELTAEDITOR_API bool CancelInFlightAnimations(EditorCore& core);

    DELTAEDITOR_API bool HasInFlightAnimations() const;
    DELTAEDITOR_API size_t GetInstanceCount() const { return m_instances.size(); }

    // Advance all animations by deltaTime. Completed scalar animations are committed to the
    // undo stack individually; completed transform channels decrement their session's channel
    // count and commit one m_localTransform SetProperty when the count reaches zero.
    DELTAEDITOR_API void Tick(float deltaTime, EditorCore& core);

private:
    EditorAnimationInstance*   FindInstance(const AssetId&, const ObjectId&, const std::string&);
    TransformAnimationSession* FindSession(const AssetId&, const ObjectId&);
    void NotifyChannelComplete(const AssetId& assetId, const ObjectId& objectId, EditorCore& core);

    std::vector<EditorAnimationInstance>        m_instances;
    std::vector<TransformAnimationChannel_Vec3> m_vec3Channels;
    std::vector<TransformAnimationChannel_Quat> m_quatChannels;
    std::vector<TransformAnimationSession>      m_sessions;
};

DELTA_ENGINE_NS_END
