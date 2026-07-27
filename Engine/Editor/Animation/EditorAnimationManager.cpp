// Engine/Editor/Animation/EditorAnimationManager.cpp
#include "Editor/Animation/EditorAnimationManager.h"

#include "Editor/EditorCore.h"
#include "Editor/Commands/EditorCommand_SetProperty.h"
#include "Editor/Commands/EditorCommandBatch.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/PropertyValueIO.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Reflection/DClass.h"

#include <algorithm>
#include <map>

using namespace DeltaEngine;
using namespace DirectX::SimpleMath;

namespace
{
// Maps a transform channel name to the decomposed reflected property it commits.
// Rotation commits the euler hint (source of truth); the quaternion is rebuilt from it.
const char* PropertyNameForChannel(const std::string& channelName)
{
    if (channelName == "position") return "m_localPosition";
    if (channelName == "scale")    return "m_localScale";
    if (channelName == "rotation") return "m_localEulerAngles";
    return nullptr;
}
} // namespace

// ─── Scalar (float) — unchanged from Phase 1 ───────────────────────────────

void EditorAnimationManager::StartAnimation(
    AssetId assetId, ObjectId objectId, std::string propertyName,
    float currentValue, float targetValue, float duration,
    std::function<void(float)> setter)
{
    EditorAnimationInstance* existing = FindInstance(assetId, objectId, propertyName);
    if (existing)
    {
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

// ─── Vec3 channels ─────────────────────────────────────────────────────────

void EditorAnimationManager::StartAnimationVec3(
    AssetId assetId, ObjectId objectId, std::string channelName,
    Vector3 fromValue, Vector3 targetValue, float duration,
    std::function<void(Vector3)> setter,
    const nlohmann::json& channelPropertyBefore)
{
    const auto existingIt = std::find_if(m_vec3Channels.begin(), m_vec3Channels.end(),
        [&](const auto& ch) { return ch.MatchesKey(assetId, objectId, channelName); });

    if (existingIt != m_vec3Channels.end())
    {
        // Preemption — restart from current interpolated value, session unchanged.
        existingIt->m_fromValue   = existingIt->GetCurrentValue();
        existingIt->m_targetValue = targetValue;
        existingIt->m_duration    = duration;
        existingIt->m_elapsed     = 0.0f;
        existingIt->m_complete    = false;
        existingIt->m_setter      = std::move(setter);
        return;
    }

    const char* propName = PropertyNameForChannel(channelName);

    TransformAnimationChannel_Vec3 ch;
    ch.assetId       = assetId;
    ch.objectId      = objectId;
    ch.channelName   = std::move(channelName);
    ch.m_fromValue   = fromValue;
    ch.m_targetValue = targetValue;
    ch.m_duration    = duration;
    ch.m_setter      = std::move(setter);
    m_vec3Channels.push_back(std::move(ch));

    TransformAnimationSession* session = FindSession(assetId, objectId);
    if (session)
        session->activeChannelCount++;
    else
    {
        TransformAnimationSession created;
        created.assetId            = assetId;
        created.objectId           = objectId;
        created.activeChannelCount = 1;
        m_sessions.push_back(std::move(created));
        session = &m_sessions.back();
    }

    // Record this channel's pre-session before-value (emplace preserves the original snapshot if
    // the channel completed and later re-joined the same session).
    if (propName)
        session->channelSnapshots.emplace(propName, channelPropertyBefore);
}

// ─── Quat channels ─────────────────────────────────────────────────────────

void EditorAnimationManager::StartAnimationQuat(
    AssetId assetId, ObjectId objectId, std::string channelName,
    Quaternion fromValue, Quaternion targetValue, float duration,
    std::function<void(Quaternion)> setter,
    const nlohmann::json& channelPropertyBefore)
{
    const auto existingIt = std::find_if(m_quatChannels.begin(), m_quatChannels.end(),
        [&](const auto& ch) { return ch.MatchesKey(assetId, objectId, channelName); });

    if (existingIt != m_quatChannels.end())
    {
        existingIt->m_fromValue   = existingIt->GetCurrentValue();
        existingIt->m_targetValue = targetValue;
        existingIt->m_duration    = duration;
        existingIt->m_elapsed     = 0.0f;
        existingIt->m_complete    = false;
        existingIt->m_setter      = std::move(setter);
        return;
    }

    const char* propName = PropertyNameForChannel(channelName);

    TransformAnimationChannel_Quat ch;
    ch.assetId       = assetId;
    ch.objectId      = objectId;
    ch.channelName   = std::move(channelName);
    ch.m_fromValue   = fromValue;
    ch.m_targetValue = targetValue;
    ch.m_duration    = duration;
    ch.m_setter      = std::move(setter);
    m_quatChannels.push_back(std::move(ch));

    TransformAnimationSession* session = FindSession(assetId, objectId);
    if (session)
        session->activeChannelCount++;
    else
    {
        TransformAnimationSession created;
        created.assetId            = assetId;
        created.objectId           = objectId;
        created.activeChannelCount = 1;
        m_sessions.push_back(std::move(created));
        session = &m_sessions.back();
    }

    // Record this channel's pre-session before-value (emplace preserves the original snapshot if
    // the channel completed and later re-joined the same session).
    if (propName)
        session->channelSnapshots.emplace(propName, channelPropertyBefore);
}

// ─── Drop transforms ───────────────────────────────────────────────────────

void EditorAnimationManager::DropTransformAnimations(
    const AssetId& assetId, const ObjectId& objectId)
{
    m_vec3Channels.erase(
        std::remove_if(m_vec3Channels.begin(), m_vec3Channels.end(),
            [&](const auto& ch) { return ch.assetId == assetId && ch.objectId == objectId; }),
        m_vec3Channels.end());

    m_quatChannels.erase(
        std::remove_if(m_quatChannels.begin(), m_quatChannels.end(),
            [&](const auto& ch) { return ch.assetId == assetId && ch.objectId == objectId; }),
        m_quatChannels.end());

    m_sessions.erase(
        std::remove_if(m_sessions.begin(), m_sessions.end(),
            [&](const auto& s) { return s.MatchesObject(assetId, objectId); }),
        m_sessions.end());
}

// ─── Cancel ────────────────────────────────────────────────────────────────

bool EditorAnimationManager::CancelInFlightAnimations(EditorCore& core)
{
    if (m_instances.empty() && m_vec3Channels.empty() && m_quatChannels.empty())
        return false;

    // Revert scalar animations.
    for (auto& inst : m_instances)
        inst.m_setter(inst.m_undoValue);
    m_instances.clear();

    // Restore each SceneComponent's decomposed transform properties from the per-channel snapshots.
    for (const auto& session : m_sessions)
    {
        auto* sc = core.ResolveObject<SceneComponent>(session.assetId, session.objectId);
        if (!sc)
            continue;
        for (const auto& [propName, before] : session.channelSnapshots)
        {
            if (DProperty* prop = sc->GetClass()->FindPropertyByName(propName))
                SetPropertyFromJson(sc, prop, before);
        }
    }

    m_vec3Channels.clear();
    m_quatChannels.clear();
    m_sessions.clear();
    return true;
}

bool EditorAnimationManager::HasInFlightAnimations() const
{
    return !m_instances.empty() || !m_vec3Channels.empty() || !m_quatChannels.empty();
}

// ─── Tick ──────────────────────────────────────────────────────────────────

void EditorAnimationManager::Tick(float deltaTime, EditorCore& core)
{
    // ── Scalar ────────────────────────────────────────────────────────────
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

    for (const auto& inst : completed)
    {
        EditorCommandContext ctx{core};
        auto cmd = std::make_unique<EditorCommand_SetProperty>(
            inst.m_assetId, inst.m_objectId, inst.m_propertyName,
            nlohmann::json(inst.m_undoValue),
            nlohmann::json(inst.m_targetValue));
        core.GetCommandManager().Execute(std::move(cmd), ctx);
    }

    // ── Vec3 ──────────────────────────────────────────────────────────────
    std::vector<std::pair<AssetId, ObjectId>> completedVec3;
    for (auto& ch : m_vec3Channels)
    {
        ch.Tick(deltaTime);
        if (ch.m_complete)
            completedVec3.emplace_back(ch.assetId, ch.objectId);
    }
    m_vec3Channels.erase(
        std::remove_if(m_vec3Channels.begin(), m_vec3Channels.end(),
            [](const auto& ch) { return ch.m_complete; }),
        m_vec3Channels.end());

    // ── Quat ──────────────────────────────────────────────────────────────
    std::vector<std::pair<AssetId, ObjectId>> completedQuat;
    for (auto& ch : m_quatChannels)
    {
        ch.Tick(deltaTime);
        if (ch.m_complete)
            completedQuat.emplace_back(ch.assetId, ch.objectId);
    }
    m_quatChannels.erase(
        std::remove_if(m_quatChannels.begin(), m_quatChannels.end(),
            [](const auto& ch) { return ch.m_complete; }),
        m_quatChannels.end());

    // ── Session notifications ─────────────────────────────────────────────
    for (auto& [aid, oid] : completedVec3)
        NotifyChannelComplete(aid, oid, core);
    for (auto& [aid, oid] : completedQuat)
        NotifyChannelComplete(aid, oid, core);
}

// ─── Helpers ───────────────────────────────────────────────────────────────

void EditorAnimationManager::NotifyChannelComplete(
    const AssetId& assetId, const ObjectId& objectId, EditorCore& core)
{
    auto it = std::find_if(m_sessions.begin(), m_sessions.end(),
        [&](const auto& s) { return s.MatchesObject(assetId, objectId); });
    if (it == m_sessions.end())
        return;

    if (--it->activeChannelCount > 0)
        return;

    std::map<std::string, nlohmann::json> snapshots = std::move(it->channelSnapshots);
    m_sessions.erase(it);

    // All channels done — commit one per-channel SetProperty for each participating decomposed
    // property, batched into a single undo entry.
    auto* sc = core.ResolveObject<SceneComponent>(assetId, objectId);
    if (!sc)
        return;

    auto batch = std::make_unique<EditorCommandBatch>("Animate Transform");
    int commandCount = 0;
    for (const auto& [propName, before] : snapshots)
    {
        DProperty* prop = sc->GetClass()->FindPropertyByName(propName);
        if (!prop)
            continue;
        batch->Add(std::make_unique<EditorCommand_SetProperty>(
            assetId, objectId, propName, before, PropertyToJson(sc, prop)));
        ++commandCount;
    }

    if (commandCount > 0)
    {
        EditorCommandContext ctx{core};
        core.GetCommandManager().Execute(std::move(batch), ctx);
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

TransformAnimationSession* EditorAnimationManager::FindSession(
    const AssetId& assetId, const ObjectId& objectId)
{
    auto it = std::find_if(m_sessions.begin(), m_sessions.end(),
        [&](const auto& s) { return s.MatchesObject(assetId, objectId); });
    return (it != m_sessions.end()) ? &*it : nullptr;
}
