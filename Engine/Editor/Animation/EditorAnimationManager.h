// Engine/Editor/Animation/EditorAnimationManager.h
#pragma once

#include "EditorIncludes.h"

#include "Runtime/Core/UUID.h"
#include "Editor/Animation/EditorAnimationInstance.h"
#include "Editor/Animation/TransformAnimationTypes.h"
#include "Editor/Animation/ViewportCameraAnimation.h"

#include <DirectXMath.h>
#include <SimpleMath.h>
#include <nlohmann/json.hpp>

#include <functional>
#include <optional>
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
    // channelPropertyBefore: serialized pre-session value of this channel's decomposed property
    // (m_localPosition / m_localScale), recorded into the session's per-channel snapshot map.
    DELTAEDITOR_API void StartAnimationVec3(
        AssetId assetId, ObjectId objectId, std::string channelName,
        DirectX::SimpleMath::Vector3 fromValue,
        DirectX::SimpleMath::Vector3 targetValue,
        float duration,
        std::function<void(DirectX::SimpleMath::Vector3)> setter,
        const nlohmann::json& channelPropertyBefore);

    // Quaternion transform channel (channel name "rotation").
    // channelPropertyBefore: serialized pre-session value of m_localEulerAngles (the rotation
    // source of truth), recorded into the session's per-channel snapshot map.
    DELTAEDITOR_API void StartAnimationQuat(
        AssetId assetId, ObjectId objectId, std::string channelName,
        DirectX::SimpleMath::Quaternion fromValue,
        DirectX::SimpleMath::Quaternion targetValue,
        float duration,
        std::function<void(DirectX::SimpleMath::Quaternion)> setter,
        const nlohmann::json& channelPropertyBefore);

    // Silently remove a scalar animation for the given key WITHOUT reverting.
    // Called by EditorCommand_SetProperty::Execute so a direct property write wins.
    DELTAEDITOR_API void DropAnimation(
        const AssetId& assetId, const ObjectId& objectId, const std::string& propertyName);

    // Drop all Vec3/Quat channels and the session for (assetId, objectId) without reverting.
    // Called when a direct SetProperty on a decomposed transform property should win.
    DELTAEDITOR_API void DropTransformAnimations(
        const AssetId& assetId, const ObjectId& objectId);

    // Cancel ALL in-flight animations: revert scalar channels to their undoValue and restore each
    // transform session's decomposed properties from its per-channel snapshots via
    // SetPropertyFromJson. Returns true if anything was cancelled. Called by EditorCommandManager::Undo.
    DELTAEDITOR_API bool CancelInFlightAnimations(EditorCore& core);

    // Tween the editor preview (fly) camera. Only the requested channels animate; the others —
    // and nearPlane/farPlane — stay frozen at their current value. Nothing is committed to the
    // undo stack: the viewport camera is editor-only state, not a reflected DObject.
    // Preempting an in-flight tween restarts from the currently sampled camera.
    DELTAEDITOR_API void StartViewportCameraAnimation(
        const EditorViewportCamera& from, const EditorViewportCamera& target,
        bool animateFov, bool animatePosition, bool animateRotation,
        float duration,
        std::function<void(const EditorViewportCamera&)> setter);

    // Drop the viewport camera tween WITHOUT reverting — the camera keeps the value it reached.
    // Called when the user takes manual control of the preview camera.
    DELTAEDITOR_API void CancelViewportCameraAnimation();

    DELTAEDITOR_API bool HasViewportCameraAnimation() const;

    // Note: the viewport camera tween is deliberately excluded from HasInFlightAnimations and
    // CancelInFlightAnimations — those gate EditorCommandManager::Undo, and a non-undoable camera
    // move must not swallow the user's Ctrl+Z.
    DELTAEDITOR_API bool HasInFlightAnimations() const;
    DELTAEDITOR_API size_t GetInstanceCount() const { return m_instances.size(); }

    // Advance all animations by deltaTime. Completed scalar animations are committed to the
    // undo stack individually; completed transform channels decrement their session's channel
    // count and, when it reaches zero, commit one per-channel SetProperty for each participating
    // decomposed property (batched into a single undo entry).
    DELTAEDITOR_API void Tick(float deltaTime, EditorCore& core);

private:
    EditorAnimationInstance*   FindInstance(const AssetId&, const ObjectId&, const std::string&);
    TransformAnimationSession* FindSession(const AssetId&, const ObjectId&);
    void NotifyChannelComplete(const AssetId& assetId, const ObjectId& objectId, EditorCore& core);

    std::vector<EditorAnimationInstance>        m_instances;
    std::vector<TransformAnimationChannel_Vec3> m_vec3Channels;
    std::vector<TransformAnimationChannel_Quat> m_quatChannels;
    std::vector<TransformAnimationSession>      m_sessions;
    std::optional<ViewportCameraAnimation>      m_viewportCameraAnimation;
};

DELTA_ENGINE_NS_END
