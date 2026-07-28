// Engine/Editor/Animation/ViewportCameraAnimation.h
#pragma once

#include "EditorIncludes.h"

#include "Editor/Animation/Easing.h"
#include "Editor/EditorViewportCamera.h"

#include <DirectXMath.h>
#include <SimpleMath.h>

#include <functional>

DELTA_ENGINE_NS_BEGIN

/**
 * Tweens the editor preview (fly) camera's fov / position / rotation channels.
 * Unlike the transform channels this commits nothing to the undo stack — the viewport camera is
 * editor-only state, not a reflected DObject, and SetViewportCamera is declared non-undoable.
 */
struct ViewportCameraAnimation
{
    /// Carries nearPlane/farPlane and any channel that is not being animated.
    EditorViewportCamera m_base;

    bool m_animateFov      = false;
    bool m_animatePosition = false;
    bool m_animateRotation = false;

    float m_fromFov   = 60.f;
    float m_targetFov = 60.f;

    DirectX::SimpleMath::Vector3 m_fromPosition;
    DirectX::SimpleMath::Vector3 m_targetPosition;

    DirectX::SimpleMath::Quaternion m_fromRotation;
    DirectX::SimpleMath::Quaternion m_targetRotation;

    float m_elapsed  = 0.0f;
    float m_duration = 1.0f;
    bool  m_complete = false;

    std::function<void(const EditorViewportCamera&)> m_setter;

    /// Returns the eased camera at the current time without applying it.
    EditorViewportCamera GetCurrentCamera() const
    {
        const float t = (m_duration > 0.0f) ? (m_elapsed / m_duration) : 1.0f;
        return SampleAt(t);
    }

    /// Advances time, applies the sampled camera, marks complete when elapsed >= duration.
    void Tick(float dt)
    {
        m_elapsed += dt;
        const float t = (m_duration > 0.0f) ? (m_elapsed / m_duration) : 1.0f;
        m_setter(SampleAt(t));
        if (t >= 1.0f)
            m_complete = true;
    }

private:
    EditorViewportCamera SampleAt(float t) const
    {
        EditorViewportCamera cam = m_base;
        const bool done = (t >= 1.0f);
        const float e   = done ? 1.0f : CubicEaseOut(t);

        if (m_animateFov)
            cam.fov = done ? m_targetFov : (m_fromFov + e * (m_targetFov - m_fromFov));

        if (m_animatePosition)
        {
            const DirectX::SimpleMath::Vector3 p =
                done ? m_targetPosition
                     : DirectX::SimpleMath::Vector3::Lerp(m_fromPosition, m_targetPosition, e);
            cam.position = { p.x, p.y, p.z };
        }

        if (m_animateRotation)
        {
            DirectX::SimpleMath::Quaternion q = m_targetRotation;
            if (!done)
            {
                // Negate to take the shortest arc if needed.
                if (m_fromRotation.Dot(q) < 0.0f)
                    q = DirectX::SimpleMath::Quaternion(-q.x, -q.y, -q.z, -q.w);
                q = DirectX::SimpleMath::Quaternion::Slerp(m_fromRotation, q, e);
            }
            cam.rotation = { q.x, q.y, q.z, q.w };
        }

        return cam;
    }
};

DELTA_ENGINE_NS_END
