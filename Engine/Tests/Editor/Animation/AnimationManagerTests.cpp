// Engine/Tests/Editor/Animation/AnimationManagerTests.cpp
#include "Editor/EditorCoreFixture.h"

#include "Editor/Animation/EditorAnimationManager.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/EditorViewportCamera.h"

#include "Runtime/Core/UUID.h"

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;
using DeltaEngine::UUID;

class AnimationManagerTests : public EditorCoreFixture {};

static const AssetId  kNullAsset  = UUID::Null();
static const ObjectId kNullObject = UUID::Null();

// Tick at t=0: value should equal start
TEST_F(AnimationManagerTests, Tick_AtZero_ValueEqualsStart)
{
    float value = 99.0f;
    EditorAnimationManager mgr;
    mgr.StartAnimation(kNullAsset, kNullObject, "m_test", 0.0f, 10.0f, 1.0f,
        [&value](float v) { value = v; });

    mgr.Tick(0.0f, *m_core);
    EXPECT_NEAR(value, 0.0f, 0.001f);
}

// Tick at half duration: CubicEaseOut(0.5) = 0.875 → value = 8.75
TEST_F(AnimationManagerTests, Tick_AtHalfDuration_ValueMatchesCubicEaseOut)
{
    float value = 0.0f;
    EditorAnimationManager mgr;
    mgr.StartAnimation(kNullAsset, kNullObject, "m_test", 0.0f, 10.0f, 2.0f,
        [&value](float v) { value = v; });

    mgr.Tick(1.0f, *m_core);
    EXPECT_NEAR(value, 8.75f, 0.01f);
}

// On complete: snaps to exact target
TEST_F(AnimationManagerTests, Tick_OnComplete_SnapsToTargetValue)
{
    float value = 0.0f;
    EditorAnimationManager mgr;
    mgr.StartAnimation(kNullAsset, kNullObject, "m_test", 0.0f, 7.0f, 1.0f,
        [&value](float v) { value = v; });

    mgr.Tick(2.0f, *m_core);
    EXPECT_NEAR(value, 7.0f, 0.001f);
}

// On complete: instance is removed
TEST_F(AnimationManagerTests, Tick_OnComplete_InstanceIsRemoved)
{
    EditorAnimationManager mgr;
    mgr.StartAnimation(kNullAsset, kNullObject, "m_test", 0.0f, 1.0f, 1.0f,
        [](float) {});

    mgr.Tick(2.0f, *m_core);
    EXPECT_FALSE(mgr.HasInFlightAnimations());
}

// On complete with null IDs: no crash (command won't push, that's ok)
TEST_F(AnimationManagerTests, Tick_OnComplete_NoCrashWithNullIds)
{
    EditorAnimationManager mgr;
    mgr.StartAnimation(kNullAsset, kNullObject, "m_test", 0.0f, 5.0f, 0.5f,
        [](float) {});
    EXPECT_NO_FATAL_FAILURE(mgr.Tick(1.0f, *m_core));
}

// Cancel with no animations: returns false
TEST_F(AnimationManagerTests, Cancel_ReturnsFalse_WhenNoAnimations)
{
    EditorAnimationManager mgr;
    EXPECT_FALSE(mgr.CancelInFlightAnimations(*m_core));
}

// Cancel reverts value to undoValue
TEST_F(AnimationManagerTests, Cancel_RevertsToUndoValue)
{
    float value = 5.0f;
    EditorAnimationManager mgr;
    mgr.StartAnimation(kNullAsset, kNullObject, "m_test", 5.0f, 15.0f, 2.0f,
        [&value](float v) { value = v; });

    mgr.Tick(0.5f, *m_core);
    EXPECT_GT(value, 5.0f);

    const bool cancelled = mgr.CancelInFlightAnimations(*m_core);
    EXPECT_TRUE(cancelled);
    EXPECT_NEAR(value, 5.0f, 0.001f);
}

// Cancel clears all instances
TEST_F(AnimationManagerTests, Cancel_ClearsInstances)
{
    EditorAnimationManager mgr;
    mgr.StartAnimation(kNullAsset, kNullObject, "m_test", 0.0f, 1.0f, 1.0f,
        [](float) {});
    mgr.CancelInFlightAnimations(*m_core);
    EXPECT_FALSE(mgr.HasInFlightAnimations());
}

// Preempt: same key does NOT create a second instance
TEST_F(AnimationManagerTests, Preempt_DoesNotCreateSecondInstance)
{
    EditorAnimationManager mgr;
    mgr.StartAnimation(kNullAsset, kNullObject, "m_test", 0.0f, 10.0f, 2.0f,
        [](float) {});
    mgr.Tick(0.5f, *m_core);

    mgr.StartAnimation(kNullAsset, kNullObject, "m_test", 0.0f, 20.0f, 1.0f,
        [](float) {});

    EXPECT_EQ(mgr.GetInstanceCount(), 1u);
}

// Preempt: cancel reverts to original undoValue (not the mid-tween value)
TEST_F(AnimationManagerTests, Preempt_PreservesUndoValue)
{
    float value = 3.0f;
    EditorAnimationManager mgr;
    mgr.StartAnimation(kNullAsset, kNullObject, "m_test", 3.0f, 10.0f, 2.0f,
        [&value](float v) { value = v; });
    mgr.Tick(1.0f, *m_core);

    const float midValue = value;
    EXPECT_GT(midValue, 3.0f);

    mgr.StartAnimation(kNullAsset, kNullObject, "m_test", midValue, 20.0f, 1.0f,
        [&value](float v) { value = v; });

    mgr.CancelInFlightAnimations(*m_core);
    EXPECT_NEAR(value, 3.0f, 0.001f);
}

// Drop: removes instance without reverting value
TEST_F(AnimationManagerTests, Drop_RemovesInstanceWithoutReverting)
{
    float value = 0.0f;
    EditorAnimationManager mgr;
    mgr.StartAnimation(kNullAsset, kNullObject, "m_test", 0.0f, 10.0f, 2.0f,
        [&value](float v) { value = v; });
    mgr.Tick(0.5f, *m_core);
    const float mid = value;

    mgr.DropAnimation(kNullAsset, kNullObject, "m_test");

    EXPECT_FALSE(mgr.HasInFlightAnimations());
    EXPECT_NEAR(value, mid, 0.001f);  // NOT reverted
}

// ─── Viewport camera tween ─────────────────────────────────────────────────

namespace
{
EditorViewportCamera MakeCamera(float fov, float x, float nearPlane = 0.1f, float farPlane = 10000.f)
{
    EditorViewportCamera cam;
    cam.fov       = fov;
    cam.nearPlane = nearPlane;
    cam.farPlane  = farPlane;
    cam.position  = { x, 0.f, 0.f };
    cam.rotation  = { 0.f, 0.f, 0.f, 1.f };
    return cam;
}
} // namespace

// Half duration: CubicEaseOut(0.5) = 0.875 for both fov and position
TEST_F(AnimationManagerTests, ViewportCamera_AtHalfDuration_MatchesCubicEaseOut)
{
    EditorViewportCamera applied;
    EditorAnimationManager mgr;
    mgr.StartViewportCameraAnimation(MakeCamera(60.f, 0.f), MakeCamera(100.f, 10.f),
        true, true, false, 2.0f,
        [&applied](const EditorViewportCamera& c) { applied = c; });

    mgr.Tick(1.0f, *m_core);

    EXPECT_NEAR(applied.fov, 95.0f, 0.01f);
    EXPECT_NEAR(applied.position.x, 8.75f, 0.01f);
}

// On complete: snaps to the exact target and the tween is dropped
TEST_F(AnimationManagerTests, ViewportCamera_OnComplete_SnapsToTargetAndClears)
{
    EditorViewportCamera applied;
    EditorAnimationManager mgr;
    mgr.StartViewportCameraAnimation(MakeCamera(60.f, 0.f), MakeCamera(90.f, 7.f),
        true, true, false, 1.0f,
        [&applied](const EditorViewportCamera& c) { applied = c; });

    mgr.Tick(2.0f, *m_core);

    EXPECT_NEAR(applied.fov, 90.0f, 0.001f);
    EXPECT_NEAR(applied.position.x, 7.0f, 0.001f);
    EXPECT_FALSE(mgr.HasViewportCameraAnimation());
}

// Non-animated channels and near/far planes are carried through untouched
TEST_F(AnimationManagerTests, ViewportCamera_LeavesUnrequestedChannelsUntouched)
{
    EditorViewportCamera applied;
    EditorAnimationManager mgr;
    mgr.StartViewportCameraAnimation(MakeCamera(60.f, 3.f, 0.5f, 500.f), MakeCamera(120.f, 42.f),
        true, false, false, 1.0f,
        [&applied](const EditorViewportCamera& c) { applied = c; });

    mgr.Tick(0.5f, *m_core);

    EXPECT_NEAR(applied.position.x, 3.0f, 0.001f);  // position not requested
    EXPECT_NEAR(applied.nearPlane, 0.5f, 0.001f);
    EXPECT_NEAR(applied.farPlane, 500.f, 0.001f);
}

// The tween is non-undoable: it never registers as in-flight and never pushes a command
TEST_F(AnimationManagerTests, ViewportCamera_IsNotUndoableAndDoesNotBlockUndo)
{
    EditorAnimationManager mgr;
    mgr.StartViewportCameraAnimation(MakeCamera(60.f, 0.f), MakeCamera(90.f, 5.f),
        true, true, false, 1.0f,
        [](const EditorViewportCamera&) {});

    EXPECT_TRUE(mgr.HasViewportCameraAnimation());
    EXPECT_FALSE(mgr.HasInFlightAnimations());
    EXPECT_FALSE(mgr.CancelInFlightAnimations(*m_core));

    mgr.Tick(2.0f, *m_core);
    EXPECT_FALSE(m_core->GetCommandManager().CanUndo());
}

// Cancel drops the tween without reverting — the camera keeps the value it reached
TEST_F(AnimationManagerTests, ViewportCamera_Cancel_DropsWithoutReverting)
{
    EditorViewportCamera applied;
    EditorAnimationManager mgr;
    mgr.StartViewportCameraAnimation(MakeCamera(60.f, 0.f), MakeCamera(60.f, 10.f),
        false, true, false, 2.0f,
        [&applied](const EditorViewportCamera& c) { applied = c; });

    mgr.Tick(0.5f, *m_core);
    const float mid = applied.position.x;
    EXPECT_GT(mid, 0.0f);

    mgr.CancelViewportCameraAnimation();

    EXPECT_FALSE(mgr.HasViewportCameraAnimation());
    mgr.Tick(1.0f, *m_core);
    EXPECT_NEAR(applied.position.x, mid, 0.001f);
}

// Preemption restarts from the currently sampled camera rather than the caller's "from"
TEST_F(AnimationManagerTests, ViewportCamera_Preempt_RestartsFromSampledCamera)
{
    EditorViewportCamera applied;
    EditorAnimationManager mgr;
    mgr.StartViewportCameraAnimation(MakeCamera(60.f, 0.f), MakeCamera(60.f, 10.f),
        false, true, false, 2.0f,
        [&applied](const EditorViewportCamera& c) { applied = c; });

    mgr.Tick(1.0f, *m_core);
    const float mid = applied.position.x;

    // A caller passing a stale "from" must not cause a visible jump on the next tick.
    mgr.StartViewportCameraAnimation(MakeCamera(60.f, 0.f), MakeCamera(60.f, 20.f),
        false, true, false, 2.0f,
        [&applied](const EditorViewportCamera& c) { applied = c; });

    mgr.Tick(0.0f, *m_core);
    EXPECT_NEAR(applied.position.x, mid, 0.001f);
}
