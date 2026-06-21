#include "Runtime/Core/SceneComponent.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(SceneComponent, SceneComponent_SetLocalScale_WithScaledParent_PreservesRequestedLocalScale)
{
    SceneComponent parent;
    SceneComponent child;

    parent.SetLocalScale(2.f, 2.f, 2.f);

    child.SetParent(&parent);
    child.SetLocalPosition(DirectX::SimpleMath::Vector3 { 1.f, 0.f, 0.f });

    child.SetLocalScale(3.f, 4.f, 5.f);

    const DirectX::SimpleMath::Vector3 ls = child.GetLocalScale();
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

    // World-space move on a parented child derives the local position by subtracting
    // the parent position; without forcing w=1 this corrupts the homogeneous transform
    // (r[3].w becomes 0), which collapses the mesh under perspective projection.
    child.SetWorldPosition(DirectX::SimpleMath::Vector3 { 5.f, 2.f, -3.f });

    const DirectX::XMMATRIX world = child.GetWorldTransform();
    EXPECT_NEAR(DirectX::XMVectorGetW(world.r[3]), 1.f, 1e-4f);

    const DirectX::SimpleMath::Vector3 wp = child.GetWorldPosition();
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

    const DirectX::SimpleMath::Vector3 wp = child.GetWorldPosition();
    EXPECT_NEAR(wp.x, 2.f, 1e-4f);
    EXPECT_NEAR(wp.y, 0.f, 1e-4f);
    EXPECT_NEAR(wp.z, 0.f, 1e-4f);
}
