#include "Runtime/Reflection/DObjectReferenceTraversal.h"
#include "Runtime/Reflection/DStruct.h"
#include "Runtime/Core/UUID.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DVectorProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Test/ReflectionTestObject.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

class DTexture;

TEST(ReflectionReferenceTraversalTests, VisitStruct_NullSchema_IsNoOp)
{
    ReflectionTestObject obj {};

    VisitUnresolvedObjectReferencesInStruct(nullptr, &obj,
        [&](DObjectPtrPropertyBase*, void*, const ScriptPointer&)
        {
            FAIL();
        });
}

TEST(ReflectionReferenceTraversalTests, SurfaceUnresolved_OnObjectPointer_TriggersVisitor)
{
    auto& registry = GetReflectionRegistry();
    DClass* cls = registry.FindClassByName("ReflectionTestObject");
    ASSERT_NE(cls, nullptr);
    ReflectionTestObject* obj =
        registry.CreateObject<ReflectionTestObject>("ReflectionTestObject");
    ASSERT_NE(obj, nullptr);

    DProperty* texProp = cls->FindPropertyByName("m_rTexture");
    ASSERT_NE(texProp, nullptr);
    auto* ptrProp = dynamic_cast<DObjectPtrPropertyBase*>(texProp);
    ASSERT_NE(ptrProp, nullptr);

    ScriptPointer sp;
    sp.m_objectId = ObjectId::Generate();
    sp.m_assetId = AssetId::Generate();

    void* texSlot = ptrProp->GetValue(obj);
    ptrProp->SetUnresolvedPointer(texSlot, sp);

    int hitCount = 0;
    VisitUnresolvedObjectReferencesInStruct(cls, obj,
        [&](DObjectPtrPropertyBase* pp, void* addr, const ScriptPointer& rsp)
        {
            if (pp == ptrProp && addr == texSlot)
            {
                EXPECT_EQ(rsp, sp);
                ++hitCount;
            }
        });

    EXPECT_EQ(hitCount, 1);

    registry.DestroyObject(obj);
}

TEST(ReflectionReferenceTraversalTests, VisitVectorObjectPointer_ElementsObserved)
{
    auto& registry = GetReflectionRegistry();
    DClass* cls = registry.FindClassByName("ReflectionTestObject");
    ASSERT_NE(cls, nullptr);
    ReflectionTestObject* obj =
        registry.CreateObject<ReflectionTestObject>("ReflectionTestObject");
    ASSERT_NE(obj, nullptr);

    DProperty* vecProp = cls->FindPropertyByName("m_rTexVec");
    ASSERT_NE(vecProp, nullptr);
    auto* vecBase = dynamic_cast<DVectorPropertyBase*>(vecProp);
    ASSERT_NE(vecBase, nullptr);

    DProperty* inner = const_cast<DProperty*>(vecBase->GetInnerProperty());
    ASSERT_NE(inner, nullptr);
    auto* innerPtr = dynamic_cast<DObjectPtrPropertyBase*>(inner);
    ASSERT_NE(innerPtr, nullptr);

    std::vector<DTexture*> vec(1, nullptr);
    vecProp->SetValue(obj, &vec);

    void* vecAddr = vecProp->GetValue(obj);
    void* el = vecBase->GetElementAddress(vecAddr, 0);
    ASSERT_NE(el, nullptr);

    ScriptPointer sp;
    sp.m_objectId = ObjectId::Generate();
    innerPtr->SetUnresolvedPointer(el, sp);

    int hitCount = 0;
    VisitUnresolvedObjectReferencesInStruct(cls, obj,
        [&](DObjectPtrPropertyBase* pp, void* addr, const ScriptPointer& rsp)
        {
            if (pp == innerPtr && addr == el)
            {
                EXPECT_EQ(rsp, sp);
                ++hitCount;
            }
        });

    EXPECT_EQ(hitCount, 1);

    registry.DestroyObject(obj);
}
