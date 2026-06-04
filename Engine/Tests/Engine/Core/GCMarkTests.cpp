#include "Runtime/Core/DObject.h"
#include "Runtime/Core/DTexture.h"
#include "Runtime/Core/GC/DObjectGCTypes.h"
#include "Runtime/Core/GC/DObjectRegistry.h"
#include "Runtime/Core/GC/GCManager.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Test/ReflectionTestObject.h"
#include "Runtime/Test/SnapshotTestTypes.h"

#include <gtest/gtest.h>
#include <vector>

using namespace DeltaEngine;

namespace
{
EGCMarkColor ColorOf(DObject* obj)
{
    return obj->GetGCMarkColor();
}
}

TEST(GCMarkTests, DirectReferenceClassifiesReachableSet)
{
    auto& registry = GetReflectionRegistry();

    auto* root = registry.CreateObject<DSnapshotTestComponentA>("DSnapshotTestComponentA");
    auto* referenced = registry.CreateObject<DSnapshotTestComponentB>("DSnapshotTestComponentB");
    auto* unreachable = registry.CreateObject<DSnapshotTestComponentB>("DSnapshotTestComponentB");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(referenced, nullptr);
    ASSERT_NE(unreachable, nullptr);

    root->m_sibling = referenced;

    GetDObjectRegistry().AddRoot(root->GetGCHandle());

    GetGCManager().Mark();

    EXPECT_EQ(ColorOf(root), EGCMarkColor::Black);
    EXPECT_EQ(ColorOf(referenced), EGCMarkColor::Black);
    EXPECT_EQ(ColorOf(unreachable), EGCMarkColor::White);

    GetDObjectRegistry().RemoveRoot(root->GetGCHandle());
    registry.DestroyObject(root);
    registry.DestroyObject(referenced);
    registry.DestroyObject(unreachable);
}

TEST(GCMarkTests, VectorReferencesAreMarked)
{
    auto& registry = GetReflectionRegistry();

    DClass* cls = registry.FindClassByName("ReflectionTestObject");
    ASSERT_NE(cls, nullptr);

    auto* root = registry.CreateObject<ReflectionTestObject>("ReflectionTestObject");
    auto* viaPtr = registry.CreateObject<DTexture>("DTexture");
    auto* viaVec = registry.CreateObject<DTexture>("DTexture");
    auto* unreachable = registry.CreateObject<DTexture>("DTexture");
    ASSERT_NE(root, nullptr);
    ASSERT_NE(viaPtr, nullptr);
    ASSERT_NE(viaVec, nullptr);
    ASSERT_NE(unreachable, nullptr);

    DProperty* texProp = cls->FindPropertyByName("m_rTexture");
    ASSERT_NE(texProp, nullptr);
    DTexture* texPtr = viaPtr;
    texProp->SetValue(root, &texPtr);

    DProperty* vecProp = cls->FindPropertyByName("m_rTexVec");
    ASSERT_NE(vecProp, nullptr);
    std::vector<DTexture*> vec{ viaVec };
    vecProp->SetValue(root, &vec);

    GetDObjectRegistry().AddRoot(root->GetGCHandle());

    GetGCManager().Mark();

    EXPECT_EQ(ColorOf(root), EGCMarkColor::Black);
    EXPECT_EQ(ColorOf(viaPtr), EGCMarkColor::Black);
    EXPECT_EQ(ColorOf(viaVec), EGCMarkColor::Black);
    EXPECT_EQ(ColorOf(unreachable), EGCMarkColor::White);

    GetDObjectRegistry().RemoveRoot(root->GetGCHandle());
    registry.DestroyObject(root);
    registry.DestroyObject(viaPtr);
    registry.DestroyObject(viaVec);
    registry.DestroyObject(unreachable);
}

TEST(GCMarkTests, RootedCycleTerminatesAndMarksBoth)
{
    auto& registry = GetReflectionRegistry();

    auto* a = registry.CreateObject<DSnapshotTestComponentA>("DSnapshotTestComponentA");
    auto* b = registry.CreateObject<DSnapshotTestComponentB>("DSnapshotTestComponentB");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    a->m_sibling = b;
    b->m_sibling = a;

    GetDObjectRegistry().AddRoot(a->GetGCHandle());

    GetGCManager().Mark();

    EXPECT_EQ(ColorOf(a), EGCMarkColor::Black);
    EXPECT_EQ(ColorOf(b), EGCMarkColor::Black);

    GetDObjectRegistry().RemoveRoot(a->GetGCHandle());
    registry.DestroyObject(a);
    registry.DestroyObject(b);
}

TEST(GCMarkTests, UnrootedCycleStaysWhite)
{
    auto& registry = GetReflectionRegistry();

    auto* a = registry.CreateObject<DSnapshotTestComponentA>("DSnapshotTestComponentA");
    auto* b = registry.CreateObject<DSnapshotTestComponentB>("DSnapshotTestComponentB");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);

    a->m_sibling = b;
    b->m_sibling = a;

    GetGCManager().Mark();

    EXPECT_EQ(ColorOf(a), EGCMarkColor::White);
    EXPECT_EQ(ColorOf(b), EGCMarkColor::White);

    registry.DestroyObject(a);
    registry.DestroyObject(b);
}
