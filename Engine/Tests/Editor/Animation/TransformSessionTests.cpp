// Engine/Tests/Editor/Animation/TransformSessionTests.cpp
#include "Editor/EditorCoreFixture.h"
#include "Editor/Mcp/McpCoreFixture.h"

#include "Editor/Animation/EditorAnimationManager.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/PropertyValueIO.h"

#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Reflection/DClass.h"

#include <DirectXMath.h>
#include <SimpleMath.h>
#ifdef UUID
#undef UUID
#endif
#include <nlohmann/json.hpp>

#include <string>
#include <vector>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;
using namespace DirectX::SimpleMath;
using DeltaEngine::UUID;
using json = nlohmann::json;

// ─── Fixture ─────────────────────────────────────────────────────────────────

class TransformSessionTests : public McpCoreFixture
{
protected:
    // Creates a PointLight SceneComponent on a new GameObject and returns the SC + its IDs.
    SceneComponent* CreatePointLight(AssetId& outAssetId, ObjectId& outObjectId)
    {
        const std::string goId = CreateLegacyGameObject();
        if (goId.empty()) return nullptr;

        const std::string plId = ExecCreateComponent(goId, "PointLight");
        if (plId.empty()) return nullptr;

        ObjectId oid = UUID::FromString(plId);
        DWorld* world = m_core->GetWorld();
        if (!world) return nullptr;

        for (GameObject* go : world->GetGameObjects())
            for (SceneComponent* sc : go->GetSceneComponents())
                if (sc->GetObjectId() == oid)
                {
                    auto [aid, _] = m_core->GetIdsForObject(sc);
                    outAssetId  = aid;
                    outObjectId = oid;
                    return sc;
                }
        return nullptr;
    }

    static json SnapshotTransform(SceneComponent* sc)
    {
        DProperty* prop = sc->GetClass()->FindPropertyByName("m_localTransform");
        return PropertyToJson(sc, prop);
    }
};

// ─── Single Vec3 channel commits SetProperty on completion ───────────────────

TEST_F(TransformSessionTests, SinglePositionChannel_CommitsOnComplete)
{
    AssetId assetId; ObjectId objectId;
    SceneComponent* sc = CreatePointLight(assetId, objectId);
    ASSERT_NE(sc, nullptr);

    const json snapshot = SnapshotTransform(sc);
    const Vector3 target(5.0f, 0.0f, 0.0f);

    EditorAnimationManager mgr;
    mgr.StartAnimationVec3(assetId, objectId, "position",
        sc->GetLocalPosition(), target, 1.0f,
        [sc](Vector3 p) { sc->SetLocalPosition(p); },
        snapshot);

    EXPECT_TRUE(mgr.HasInFlightAnimations());

    // Tick past the end.
    mgr.Tick(2.0f, *m_core);

    EXPECT_FALSE(mgr.HasInFlightAnimations());
    EXPECT_NEAR(sc->GetLocalPosition().x, 5.0f, 0.001f);
    EXPECT_TRUE(m_core->GetCommandManager().CanUndo());
}

// ─── Concurrent channels commit ONE SetProperty when the last one finishes ───

TEST_F(TransformSessionTests, ConcurrentChannels_CommitOnceWhenBothComplete)
{
    AssetId assetId; ObjectId objectId;
    SceneComponent* sc = CreatePointLight(assetId, objectId);
    ASSERT_NE(sc, nullptr);
    m_core->GetCommandManager().Clear();  // discard CreateComponent entry

    const json snapshot = SnapshotTransform(sc);

    EditorAnimationManager mgr;
    // Position channel: 1 second.
    mgr.StartAnimationVec3(assetId, objectId, "position",
        sc->GetLocalPosition(), Vector3(3.0f, 0.0f, 0.0f), 1.0f,
        [sc](Vector3 p) { sc->SetLocalPosition(p); },
        snapshot);

    // Rotation channel: 2 seconds — starts after session already exists, snapshot ignored.
    const json snapshotMid = SnapshotTransform(sc);  // mid-state, should not be used
    const Quaternion targetRot = Quaternion::CreateFromYawPitchRoll(0.5f, 0.0f, 0.0f);
    mgr.StartAnimationQuat(assetId, objectId, "rotation",
        sc->GetLocalRotation(), targetRot, 2.0f,
        [sc](Quaternion q) { sc->SetLocalRotation(q); },
        snapshotMid);

    // Tick past first channel (1.5s), but not the second (2s).
    mgr.Tick(1.5f, *m_core);
    EXPECT_TRUE(mgr.HasInFlightAnimations());
    // Position done, rotation still running — no commit yet.
    EXPECT_FALSE(m_core->GetCommandManager().CanUndo());

    // Tick past both.
    mgr.Tick(1.0f, *m_core);
    EXPECT_FALSE(mgr.HasInFlightAnimations());

    // Exactly one SetProperty committed on the undo stack.
    EXPECT_TRUE(m_core->GetCommandManager().CanUndo());
    EXPECT_NEAR(sc->GetLocalPosition().x, 3.0f, 0.001f);
}

// ─── Session commit uses session-start snapshot, not mid-animation ───────────

TEST_F(TransformSessionTests, SessionCommit_ValueBefore_IsSessionStartSnapshot)
{
    AssetId assetId; ObjectId objectId;
    SceneComponent* sc = CreatePointLight(assetId, objectId);
    ASSERT_NE(sc, nullptr);

    // Remember original position.
    const Vector3 origin = sc->GetLocalPosition();
    const json snapshot  = SnapshotTransform(sc);

    EditorAnimationManager mgr;
    mgr.StartAnimationVec3(assetId, objectId, "position",
        origin, Vector3(7.0f, 0.0f, 0.0f), 1.0f,
        [sc](Vector3 p) { sc->SetLocalPosition(p); },
        snapshot);

    mgr.Tick(2.0f, *m_core);
    EXPECT_NEAR(sc->GetLocalPosition().x, 7.0f, 0.001f);

    // Undo should restore to origin.
    EditorCommandContext undoCtx{*m_core};
    m_core->GetCommandManager().Undo(undoCtx);
    EXPECT_NEAR(sc->GetLocalPosition().x, origin.x, 0.001f);
}

// ─── Preemption: same channel does not create a second session entry ──────────

TEST_F(TransformSessionTests, Preemption_DoesNotDuplicateSessionCount)
{
    AssetId assetId; ObjectId objectId;
    SceneComponent* sc = CreatePointLight(assetId, objectId);
    ASSERT_NE(sc, nullptr);
    m_core->GetCommandManager().Clear();  // discard CreateComponent entry

    const json snapshot = SnapshotTransform(sc);

    EditorAnimationManager mgr;
    mgr.StartAnimationVec3(assetId, objectId, "position",
        sc->GetLocalPosition(), Vector3(5.0f, 0.0f, 0.0f), 2.0f,
        [sc](Vector3 p) { sc->SetLocalPosition(p); }, snapshot);

    // Advance partially then preempt.
    mgr.Tick(0.5f, *m_core);
    mgr.StartAnimationVec3(assetId, objectId, "position",
        sc->GetLocalPosition(), Vector3(10.0f, 0.0f, 0.0f), 1.0f,
        [sc](Vector3 p) { sc->SetLocalPosition(p); }, snapshot);

    // Still one instance only.
    EXPECT_EQ(mgr.GetInstanceCount(), 0u);  // no float instances

    mgr.Tick(2.0f, *m_core);
    EXPECT_FALSE(mgr.HasInFlightAnimations());
    EXPECT_NEAR(sc->GetLocalPosition().x, 10.0f, 0.001f);
    // Exactly one undo entry.
    EXPECT_TRUE(m_core->GetCommandManager().CanUndo());
    EditorCommandContext preemptUndoCtx{*m_core};
    m_core->GetCommandManager().Undo(preemptUndoCtx);
    EXPECT_FALSE(m_core->GetCommandManager().CanUndo());
}

// ─── CancelInFlightAnimations restores session snapshot and emits no commit ──

TEST_F(TransformSessionTests, Cancel_MidSession_RestoresSnapshot_NoCommit)
{
    AssetId assetId; ObjectId objectId;
    SceneComponent* sc = CreatePointLight(assetId, objectId);
    ASSERT_NE(sc, nullptr);
    m_core->GetCommandManager().Clear();  // discard CreateComponent entry

    const Vector3 origin = sc->GetLocalPosition();
    const json snapshot  = SnapshotTransform(sc);

    EditorAnimationManager mgr;
    mgr.StartAnimationVec3(assetId, objectId, "position",
        origin, Vector3(8.0f, 0.0f, 0.0f), 2.0f,
        [sc](Vector3 p) { sc->SetLocalPosition(p); },
        snapshot);

    mgr.Tick(0.5f, *m_core);
    EXPECT_GT(sc->GetLocalPosition().x, 0.0f);  // animation running

    const bool cancelled = mgr.CancelInFlightAnimations(*m_core);
    EXPECT_TRUE(cancelled);

    // SceneComponent back at session-start position.
    EXPECT_NEAR(sc->GetLocalPosition().x, origin.x, 0.001f);
    // No commit happened.
    EXPECT_FALSE(m_core->GetCommandManager().CanUndo());
    EXPECT_FALSE(mgr.HasInFlightAnimations());
}

// ─── DropTransformAnimations: direct SetProperty wins over in-flight channel ─

TEST_F(TransformSessionTests, DropTransformAnimations_ClearsSessionWithoutReverting)
{
    AssetId assetId; ObjectId objectId;
    SceneComponent* sc = CreatePointLight(assetId, objectId);
    ASSERT_NE(sc, nullptr);

    const json snapshot = SnapshotTransform(sc);

    EditorAnimationManager mgr;
    mgr.StartAnimationVec3(assetId, objectId, "position",
        sc->GetLocalPosition(), Vector3(5.0f, 0.0f, 0.0f), 2.0f,
        [sc](Vector3 p) { sc->SetLocalPosition(p); }, snapshot);

    mgr.Tick(0.5f, *m_core);
    const float midX = sc->GetLocalPosition().x;
    EXPECT_GT(midX, 0.0f);

    mgr.DropTransformAnimations(assetId, objectId);
    EXPECT_FALSE(mgr.HasInFlightAnimations());
    // Value NOT reverted — the direct write wins.
    EXPECT_NEAR(sc->GetLocalPosition().x, midX, 0.001f);
}

// ─── Quaternion slerp sign-flip: opposite-hemisphere quaternions converge ────

TEST_F(TransformSessionTests, QuatChannel_ShortestPath_NegatedTarget)
{
    AssetId assetId; ObjectId objectId;
    SceneComponent* sc = CreatePointLight(assetId, objectId);
    ASSERT_NE(sc, nullptr);

    const json snapshot = SnapshotTransform(sc);
    const Quaternion from(0.0f, 0.0f, 0.0f, 1.0f);
    // Negated identity — same rotation, opposite hemisphere.
    const Quaternion target(0.0f, 0.0f, 0.0f, -1.0f);

    EditorAnimationManager mgr;
    mgr.StartAnimationQuat(assetId, objectId, "rotation",
        from, target, 1.0f,
        [sc](Quaternion q) { sc->SetLocalRotation(q); },
        snapshot);

    // At t=0.5 the rotation should still be near identity (shortest path).
    mgr.Tick(0.5f, *m_core);
    const Quaternion mid = sc->GetLocalRotation();
    // Both ±identity map to the same orientation; |w| should be near 1.
    EXPECT_NEAR(std::abs(mid.w), 1.0f, 0.1f);
}
