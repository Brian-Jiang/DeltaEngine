// Engine/Editor/Animation/EditorAnimationInstance.h
#pragma once

#include "EditorIncludes.h"

#include "Runtime/Core/UUID.h"
#include "Editor/Animation/Easing.h"

#include <functional>
#include <string>

DELTA_ENGINE_NS_BEGIN

class EditorAnimationInstance
{
public:
    AssetId  m_assetId;
    ObjectId m_objectId;
    std::string m_propertyName;

    // Value before animation started — never changes after first Start, even on preemption.
    // Used to revert on cancel and as the "before" value in the committed SetProperty command.
    float m_undoValue   = 0.0f;

    float m_fromValue   = 0.0f;  // current segment start; updated on preemption
    float m_targetValue = 0.0f;
    float m_elapsed     = 0.0f;
    float m_duration    = 1.0f;
    bool  m_complete    = false;

    std::function<void(float)> m_setter;

    bool MatchesKey(const AssetId& a, const ObjectId& o, const std::string& p) const
    {
        return m_assetId == a && m_objectId == o && m_propertyName == p;
    }

    // Returns current eased value without applying it.
    float GetCurrentValue() const
    {
        if (m_duration <= 0.0f) return m_targetValue;
        float t = m_elapsed / m_duration;
        if (t >= 1.0f) return m_targetValue;
        return m_fromValue + CubicEaseOut(t) * (m_targetValue - m_fromValue);
    }

    // Advances time, calls setter, marks complete when elapsed >= duration.
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
            m_setter(m_fromValue + CubicEaseOut(t) * (m_targetValue - m_fromValue));
        }
    }
};

DELTA_ENGINE_NS_END
