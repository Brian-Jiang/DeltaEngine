#include "Runtime/Core/SceneComponent.h"

#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"

#include <gtest/gtest.h>

#include <cmath>

using namespace DeltaEngine;
using DirectX::SimpleMath::Quaternion;
using DirectX::SimpleMath::Vector3;

TEST(SceneComponent, SceneComponent_SetLocalScale_WithScaledParent_PreservesRequestedLocalScale)
{
    SceneComponent parent;
    SceneComponent child;

    parent.SetLocalScale(2.f, 2.f, 2.f);

    child.SetParent(&parent);
    child.SetLocalPosition(Vector3 { 1.f, 0.f, 0.f });

    child.SetLocalScale(3.f, 4.f, 5.f);

    const Vector3 ls = child.GetLocalScale();
    EXPECT_NEAR(ls.x, 3.f, 1e-4f);
    EXPECT_NEAR(ls.y, 4.f, 1e-4f);
    EXPECT_NEAR(ls.z, 5.f, 1e-4f);
}

TEST(SceneComponent, SceneComponent_SetParent_AncestorCycle_RejectsAndKeepsHierarchy)
{
    SceneComponent root;
    SceneComponent mid;
    SceneComponent leaf;

    mid.SetParent(&root);
    leaf.SetParent(&mid);

    EXPECT_EQ(mid.GetParent(), &root);
    EXPECT_EQ(leaf.GetParent(), &mid);

    root.SetParent(&leaf);

    EXPECT_EQ(root.GetParent(), nullptr);
    EXPECT_EQ(mid.GetParent(), &root);
    EXPECT_EQ(leaf.GetParent(), &mid);
}

TEST(SceneComponent, SceneComponent_SetWorldPosition_OnParentedChild_KeepsAffineTransform)
{
    SceneComponent root;
    SceneComponent child;
    child.SetParent(&root);

    child.SetWorldPosition(Vector3 { 5.f, 2.f, -3.f });

    const DirectX::XMMATRIX world = child.GetWorldTransform();
    EXPECT_NEAR(DirectX::XMVectorGetW(world.r[3]), 1.f, 1e-4f);

    const Vector3 wp = child.GetWorldPosition();
    EXPECT_NEAR(wp.x, 5.f, 1e-4f);
    EXPECT_NEAR(wp.y, 2.f, 1e-4f);
    EXPECT_NEAR(wp.z, -3.f, 1e-4f);
}

TEST(SceneComponent, SceneComponent_PostRestore_RecomputesWorldTransformFromLocals)
{
    SceneComponent root;
    SceneComponent child;

    child.SetParent(&root);
    child.SetLocalPosition(2.f, 0.f, 0.f);

    child.PostRestore();

    const Vector3 wp = child.GetWorldPosition();
    EXPECT_NEAR(wp.x, 2.f, 1e-4f);
    EXPECT_NEAR(wp.y, 0.f, 1e-4f);
    EXPECT_NEAR(wp.z, 0.f, 1e-4f);
}

TEST(SceneComponent, SceneComponent_SetLocalScale_Negative_RoundTripsExactly)
{
    // The old decompose-based storage could not represent a negative scale; decomposed
    // storage returns exactly what was set.
    SceneComponent sc;
    sc.SetLocalScale(-1.f, 2.f, 3.f);

    const Vector3 s = sc.GetLocalScale();
    EXPECT_NEAR(s.x, -1.f, 1e-4f);
    EXPECT_NEAR(s.y, 2.f, 1e-4f);
    EXPECT_NEAR(s.z, 3.f, 1e-4f);
}

TEST(SceneComponent, SceneComponent_SetLocalRotation_Quaternion_RoundTrips)
{
    SceneComponent sc;
    const DirectX::XMVECTOR q = DirectX::XMQuaternionRotationRollPitchYaw(0.3f, -0.7f, 1.2f);
    sc.SetLocalRotation(Quaternion(q));

    const Quaternion r = sc.GetLocalRotation();
    EXPECT_NEAR(r.x, DirectX::XMVectorGetX(q), 1e-4f);
    EXPECT_NEAR(r.y, DirectX::XMVectorGetY(q), 1e-4f);
    EXPECT_NEAR(r.z, DirectX::XMVectorGetZ(q), 1e-4f);
    EXPECT_NEAR(r.w, DirectX::XMVectorGetW(q), 1e-4f);
}

TEST(SceneComponent, SceneComponent_SetLocalRotation_Euler_PreservesHintAndDerivesQuat)
{
    SceneComponent sc;
    sc.SetLocalRotation(Vector3 { 0.f, 90.f, 0.f });

    // The euler hint is preserved verbatim...
    const Vector3 e = sc.GetLocalRotationEulerAngles();
    EXPECT_NEAR(e.x, 0.f, 1e-3f);
    EXPECT_NEAR(e.y, 90.f, 1e-3f);
    EXPECT_NEAR(e.z, 0.f, 1e-3f);

    // ...and the derived quaternion equals a 90-degree yaw (compare via |dot| == 1 to ignore sign).
    const DirectX::XMVECTOR expected =
        DirectX::XMQuaternionRotationRollPitchYaw(0.f, DirectX::XMConvertToRadians(90.f), 0.f);
    const Quaternion q = sc.GetLocalRotation();
    const float dot = std::fabs(DirectX::XMVectorGetX(DirectX::XMQuaternionDot((DirectX::XMVECTOR) q, expected)));
    EXPECT_NEAR(dot, 1.f, 1e-3f);
}

TEST(SceneComponent, SceneComponent_MoveAncestor_PropagatesToDescendantWorld)
{
    SceneComponent root;
    SceneComponent mid;
    SceneComponent leaf;

    mid.SetParent(&root);
    leaf.SetParent(&mid);

    mid.SetLocalPosition(1.f, 0.f, 0.f);
    leaf.SetLocalPosition(0.f, 1.f, 0.f);

    // Moving the root must invalidate and recompute the leaf's world transform lazily.
    root.SetLocalPosition(10.f, 0.f, 0.f);

    const Vector3 wp = leaf.GetWorldPosition();
    EXPECT_NEAR(wp.x, 11.f, 1e-4f);
    EXPECT_NEAR(wp.y, 1.f, 1e-4f);
    EXPECT_NEAR(wp.z, 0.f, 1e-4f);
}

TEST(SceneComponent, SceneComponent_SetWorldPosition_RotatedScaledParent_UsesFullInverse)
{
    // The corrected world->local math accounts for the parent's rotation and scale, not just
    // its translation.
    SceneComponent parent;
    SceneComponent child;

    parent.SetLocalScale(2.f, 2.f, 2.f);
    parent.SetLocalRotation(Vector3 { 0.f, 90.f, 0.f });

    child.SetParent(&parent);
    child.SetWorldPosition(Vector3 { 5.f, 2.f, -3.f });

    const Vector3 wp = child.GetWorldPosition();
    EXPECT_NEAR(wp.x, 5.f, 1e-3f);
    EXPECT_NEAR(wp.y, 2.f, 1e-3f);
    EXPECT_NEAR(wp.z, -3.f, 1e-3f);

    const DirectX::XMMATRIX world = child.GetWorldTransform();
    EXPECT_NEAR(DirectX::XMVectorGetW(world.r[3]), 1.f, 1e-4f);
}

TEST(SceneComponent, SceneComponent_PostEditChangeProperty_EulerRebuildsQuaternion)
{
    SceneComponent sc;

    const DProperty* eulerProp = sc.GetClass()->FindPropertyByName("m_localEulerAngles");
    ASSERT_NE(eulerProp, nullptr);

    // Simulate a reflection-driven edit of the euler property, then the post-edit notification.
    const Vector3 euler { 0.f, 90.f, 0.f };
    eulerProp->SetValue(&sc, &euler);
    sc.PostEditChangeProperty(eulerProp);

    const DirectX::XMVECTOR expected =
        DirectX::XMQuaternionRotationRollPitchYaw(0.f, DirectX::XMConvertToRadians(90.f), 0.f);
    const Quaternion q = sc.GetLocalRotation();
    const float dot = std::fabs(DirectX::XMVectorGetX(DirectX::XMQuaternionDot((DirectX::XMVECTOR) q, expected)));
    EXPECT_NEAR(dot, 1.f, 1e-3f);
}
