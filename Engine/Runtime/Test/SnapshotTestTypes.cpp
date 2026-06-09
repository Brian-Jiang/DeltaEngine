#include "Test/SnapshotTestTypes.h"

#include "Core/DWorld.h"
#include "Core/GameObject.h"
#include "Core/GC/GCManager.h"
#include "Serialization/ObjectSnapshotWriter.h"
#include "Serialization/ObjectSnapshotReader.h"

#include <cassert>
#include <cstdio>

using namespace DeltaEngine;

bool SnapshotTests::RunInternalPtrRoundTrip(DWorld* world)
{
    if (!world)
        return false;

    GameObject* go = world->CreateGameObject("SnapshotTestGO");
    auto* compA = go->AddComponent<DSnapshotTestComponentA>("TestA");
    auto* compB = go->AddComponent<DSnapshotTestComponentB>("TestB");

    compA->m_sibling = compB;
    compB->m_sibling = compA;
    compA->m_value = 123.0f;
    compB->m_count = 99;

    ObjectId goId = go->GetObjectId();
    ObjectId compAId = compA->GetObjectId();
    ObjectId compBId = compB->GetObjectId();

    ObjectSnapshotWriter writer;
    ObjectSnapshot snapshot = writer.Capture(go);

    world->DestroyGameObject(go);
    GetGCManager().CollectGarbage();

    ObjectSnapshotReader reader;
    DObject* restored = reader.Restore(snapshot, world, nullptr);

    auto* restoredGO = dynamic_cast<GameObject*>(restored);
    if (!restoredGO)
    {
        std::printf("[SnapshotTest] FAIL: InternalPtrRoundTrip - restored root is not a GameObject\n");
        return false;
    }

    if (restoredGO->GetObjectId() != goId)
    {
        std::printf("[SnapshotTest] FAIL: InternalPtrRoundTrip - GO objectId mismatch\n");
        return false;
    }

    auto& comps = restoredGO->GetComponents();
    if (comps.size() != 2)
    {
        std::printf("[SnapshotTest] FAIL: InternalPtrRoundTrip - expected 2 components, got %zu\n", comps.size());
        return false;
    }

    auto* rA = dynamic_cast<DSnapshotTestComponentA*>(comps[0]);
    auto* rB = dynamic_cast<DSnapshotTestComponentB*>(comps[1]);
    if (!rA || !rB)
    {
        rA = dynamic_cast<DSnapshotTestComponentA*>(comps[1]);
        rB = dynamic_cast<DSnapshotTestComponentB*>(comps[0]);
    }

    if (!rA || !rB)
    {
        std::printf("[SnapshotTest] FAIL: InternalPtrRoundTrip - could not find both component types\n");
        return false;
    }

    if (rA->m_value != 123.0f)
    {
        std::printf("[SnapshotTest] FAIL: InternalPtrRoundTrip - compA value mismatch: %.2f\n", rA->m_value);
        return false;
    }

    if (rB->m_count != 99)
    {
        std::printf("[SnapshotTest] FAIL: InternalPtrRoundTrip - compB count mismatch: %d\n", rB->m_count);
        return false;
    }

    if (rA->m_sibling != rB)
    {
        std::printf("[SnapshotTest] FAIL: InternalPtrRoundTrip - compA->m_sibling does not point to compB\n");
        return false;
    }

    if (rB->m_sibling != rA)
    {
        std::printf("[SnapshotTest] FAIL: InternalPtrRoundTrip - compB->m_sibling does not point to compA\n");
        return false;
    }

    std::printf("[SnapshotTest] PASS: InternalPtrRoundTrip\n");
    return true;
}

bool SnapshotTests::RunSceneComponentTransformPreservation(DWorld* world)
{
    if (!world)
        return false;

    GameObject* go = world->CreateGameObject("TransformTestGO");
    auto* root = go->AddSceneComponent<DSnapshotTestSceneComponent>("RootSC");
    auto* child = go->AddSceneComponent<DSnapshotTestSceneComponent>("ChildSC");

    root->SetLocalPosition(10.0f, 20.0f, 30.0f);
    root->SetLocalScale(2.0f, 2.0f, 2.0f);
    root->m_customData = 1.0f;

    child->m_customData = 2.0f;
    child->SetLocalPosition(5.0f, 5.0f, 5.0f);

    auto origRootWorld = root->GetWorldTransform();
    auto origChildWorld = child->GetWorldTransform();

    ObjectSnapshotWriter writer;
    ObjectSnapshot snapshot = writer.Capture(go);

    world->DestroyGameObject(go);
    GetGCManager().CollectGarbage();

    ObjectSnapshotReader reader;
    DObject* restored = reader.Restore(snapshot, world, nullptr);

    auto* restoredGO = dynamic_cast<GameObject*>(restored);
    if (!restoredGO)
    {
        std::printf("[SnapshotTest] FAIL: TransformPreservation - restored root is not a GameObject\n");
        return false;
    }

    auto& scs = restoredGO->GetSceneComponents();
    if (scs.size() < 2)
    {
        std::printf("[SnapshotTest] FAIL: TransformPreservation - expected >= 2 scene components, got %zu\n", scs.size());
        return false;
    }

    DSnapshotTestSceneComponent* rRoot = nullptr;
    DSnapshotTestSceneComponent* rChild = nullptr;
    for (auto* sc : scs)
    {
        auto* typed = dynamic_cast<DSnapshotTestSceneComponent*>(sc);
        if (!typed) continue;
        if (typed->m_customData == 1.0f)
            rRoot = typed;
        else if (typed->m_customData == 2.0f)
            rChild = typed;
    }

    if (!rRoot || !rChild)
    {
        std::printf("[SnapshotTest] FAIL: TransformPreservation - could not identify root/child scene components\n");
        return false;
    }

    auto rPos = rRoot->GetLocalPosition();
    if (std::abs(rPos.x - 10.0f) > 0.01f ||
        std::abs(rPos.y - 20.0f) > 0.01f ||
        std::abs(rPos.z - 30.0f) > 0.01f)
    {
        std::printf("[SnapshotTest] FAIL: TransformPreservation - root local position mismatch\n");
        return false;
    }

    auto cPos = rChild->GetLocalPosition();
    if (std::abs(cPos.x - 5.0f) > 0.01f ||
        std::abs(cPos.y - 5.0f) > 0.01f ||
        std::abs(cPos.z - 5.0f) > 0.01f)
    {
        std::printf("[SnapshotTest] FAIL: TransformPreservation - child local position mismatch\n");
        return false;
    }

    std::printf("[SnapshotTest] PASS: TransformPreservation\n");
    return true;
}

bool SnapshotTests::RunAll(DWorld* world)
{
    std::printf("[SnapshotTest] Running snapshot tests...\n");
    bool allPassed = true;
    allPassed &= RunInternalPtrRoundTrip(world);
    allPassed &= RunSceneComponentTransformPreservation(world);
    std::printf("[SnapshotTest] %s\n", allPassed ? "All tests passed." : "Some tests FAILED.");
    return allPassed;
}
