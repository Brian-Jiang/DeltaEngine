// Engine/Editor/Animation/TransformAnimationTypes.h
#pragma once

#include "EditorIncludes.h"

#include "Runtime/Core/UUID.h"
#include "Editor/Animation/Easing.h"

#include <DirectXMath.h>
#include <SimpleMath.h>
// Windows.h (pulled in by DirectXMath) typedefs 'UUID' as '_GUID', which conflicts
// with DeltaEngine::UUID when 'using namespace DeltaEngine' is active.
#ifdef UUID
#undef UUID
#endif
#include <nlohmann/json.hpp>

#include <functional>
#include <map>
#include <string>

DELTA_ENGINE_NS_BEGIN

// Animates a Vector3 transform channel ("position" or "scale").
struct TransformAnimationChannel_Vec3
{
    AssetId  assetId;
    ObjectId objectId;
    std::string channelName;

    DirectX::SimpleMath::Vector3 m_fromValue;
    DirectX::SimpleMath::Vector3 m_targetValue;
    float m_elapsed  = 0.0f;
    float m_duration = 1.0f;
    bool  m_complete = false;

    std::function<void(DirectX::SimpleMath::Vector3)> m_setter;

    bool MatchesKey(const AssetId& a, const ObjectId& o, const std::string& ch) const
    {
        return assetId == a && objectId == o && channelName == ch;
    }

    DirectX::SimpleMath::Vector3 GetCurrentValue() const
    {
        if (m_duration <= 0.0f) return m_targetValue;
        const float t = m_elapsed / m_duration;
        if (t >= 1.0f) return m_targetValue;
        return DirectX::SimpleMath::Vector3::Lerp(m_fromValue, m_targetValue, CubicEaseOut(t));
    }

    void Tick(float dt)
    {
        m_elapsed += dt;
        const float t = (m_duration > 0.0f) ? m_elapsed / m_duration : 1.0f;
        if (t >= 1.0f)
        {
            m_setter(m_targetValue);
            m_complete = true;
        }
        else
        {
            m_setter(DirectX::SimpleMath::Vector3::Lerp(m_fromValue, m_targetValue, CubicEaseOut(t)));
        }
    }
};

// Animates a Quaternion transform channel ("rotation") using shortest-path slerp.
struct TransformAnimationChannel_Quat
{
    AssetId  assetId;
    ObjectId objectId;
    std::string channelName;

    DirectX::SimpleMath::Quaternion m_fromValue;
    DirectX::SimpleMath::Quaternion m_targetValue;
    float m_elapsed  = 0.0f;
    float m_duration = 1.0f;
    bool  m_complete = false;

    std::function<void(DirectX::SimpleMath::Quaternion)> m_setter;

    bool MatchesKey(const AssetId& a, const ObjectId& o, const std::string& ch) const
    {
        return assetId == a && objectId == o && channelName == ch;
    }

    DirectX::SimpleMath::Quaternion GetCurrentValue() const
    {
        if (m_duration <= 0.0f) return m_targetValue;
        const float t = m_elapsed / m_duration;
        if (t >= 1.0f) return m_targetValue;
        DirectX::SimpleMath::Quaternion target = m_targetValue;
        // Negate to take the shortest arc if needed.
        if (m_fromValue.Dot(target) < 0.0f)
            target = DirectX::SimpleMath::Quaternion(-target.x, -target.y, -target.z, -target.w);
        return DirectX::SimpleMath::Quaternion::Slerp(m_fromValue, target, CubicEaseOut(t));
    }

    void Tick(float dt)
    {
        m_elapsed += dt;
        const float t = (m_duration > 0.0f) ? m_elapsed / m_duration : 1.0f;
        if (t >= 1.0f)
        {
            m_setter(m_targetValue);
            m_complete = true;
        }
        else
        {
            DirectX::SimpleMath::Quaternion target = m_targetValue;
            if (m_fromValue.Dot(target) < 0.0f)
                target = DirectX::SimpleMath::Quaternion(-target.x, -target.y, -target.z, -target.w);
            m_setter(DirectX::SimpleMath::Quaternion::Slerp(m_fromValue, target, CubicEaseOut(t)));
        }
    }
};

// Groups all active transform channels for one SceneComponent.
// Created on the first channel start; destroyed when the last channel completes.
struct TransformAnimationSession
{
    AssetId  assetId;
    ObjectId objectId;
    /** Pre-session value of each participating channel's reflected property, keyed by property
        name ("m_localPosition" / "m_localEulerAngles" / "m_localScale"). Captured when the channel
        first joins the session; used as the SetProperty valueBefore on completion / cancel. */
    std::map<std::string, nlohmann::json> channelSnapshots;
    int activeChannelCount = 0;

    bool MatchesObject(const AssetId& a, const ObjectId& o) const
    {
        return assetId == a && objectId == o;
    }
};

DELTA_ENGINE_NS_END
