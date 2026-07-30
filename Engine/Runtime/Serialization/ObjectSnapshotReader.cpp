#include "Runtime/Serialization/ObjectSnapshotReader.h"

#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Assets/IAssetDatabase.h"
#include "Runtime/Core/DComponent.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Core/DScene.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DObjectReferenceTraversal.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Serialization/ISerializationCallbackReceiver.h"
#include "Runtime/Serialization/JsonAssetArchive.h"
#include "Runtime/Serialization/ObjectSnapshot.h"
#include "Runtime/Logging/LogChannels.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

using namespace DeltaEngine;

DObject* ObjectSnapshotReader::Restore(
    const ObjectSnapshot& snapshot,
    DWorld* world,
    IAssetDatabase* db,
    DPrimaryAsset* registerWithAsset,
    DScene* addRestoredRootGameObjectToScene)
{
    if (snapshot.rootJson.empty() || !snapshot.rootJson.contains("objects"))
    {
        DLOG(LogSerialization, ELogLevel::Warning,
             "Skipped object snapshot restore: snapshot is missing objects array (root class: '{}')",
             snapshot.rootClassName);
        DELTA_ENSURE_MSG(false, "ObjectSnapshotReader::Restore requires objects array");
        return nullptr;
    }

    std::unordered_set<ObjectId> capturedSet(
        snapshot.capturedIds.begin(), snapshot.capturedIds.end());

    std::unordered_map<ObjectId, DObject*> idMap;
    std::vector<DObject*> restoredObjects;

    // Phase 1: allocate objects and deserialize properties
    JsonAssetArchive ar(snapshot.rootJson, std::filesystem::path{});
    size_t count = ar.BeginArrayLoad("objects");

    for (size_t i = 0; i < count; ++i)
    {
        std::string className = ar.BeginObjectLoad();

        DClass* dclass = GetReflectionRegistry().FindClassByName(className);
        if (!dclass)
        {
            DLOG(LogSerialization, ELogLevel::Warning,
                 "Skipped snapshot object {}: reflected class '{}' is not registered (expected DCLASS registration)",
                 i, className);
            ar.EndObject();
            continue;
        }

        DObject* obj = GetReflectionRegistry().CreateObject(className);
        if (!obj)
        {
            DLOG(LogSerialization, ELogLevel::Error,
                 "Skipped snapshot object {}: failed to create reflected class '{}' (expected registered constructor)",
                 i, className);
            DELTA_ENSURE_MSG(false, "ObjectSnapshotReader failed to create object");
            ar.EndObject();
            continue;
        }

        ObjectId oid;
        ar.Serialize("_objectId", oid);
        if (oid.IsNull())
        {
            DLOG(LogSerialization, ELogLevel::Warning,
                 "Snapshot object {} of class '{}' has null objectId (expected non-null serialized object id)",
                 i, className);
            GetReflectionRegistry().DestroyObject(obj);
            ar.EndObject();
            continue;
        }

        bool collision = false;
        if (world)
        {
            for (auto* go : world->GetGameObjects())
            {
                if (go->GetObjectId() == oid)
                {
                    collision = true;
                    break;
                }
            }
        }

        if (collision)
        {
            ObjectId fresh = ObjectId::Generate();
            DLOG(LogSerialization, ELogLevel::Warning,
                 "Remapped snapshot objectId collision for class '{}': '{}' -> '{}' (expected unique world object id)",
                 className, oid.ToString(), fresh.ToString());
            idMap[oid] = obj;
            obj->SetObjectId(fresh);
        }
        else
        {
            obj->SetObjectId(oid);
            idMap[oid] = obj;
        }

        dclass->Serialize(ar, *obj);
        ar.EndObject();

        restoredObjects.push_back(obj);
    }
    ar.EndArray();

    if (registerWithAsset)
    {
        for (DObject* obj : restoredObjects)
        {
            DELTA_VERIFY_MSG(obj != nullptr, "ObjectSnapshotReader restored object list contains null entry");
            if (obj && !obj->HasOwningAsset())
                registerWithAsset->AddObject(obj);
        }
    }

    // OnAfterDeserialize
    for (DObject* obj : restoredObjects)
    {
        DELTA_VERIFY_MSG(obj != nullptr, "ObjectSnapshotReader restored object list contains null entry");
        if (auto* cb = dynamic_cast<ISerializationCallbackReceiver*>(obj))
            cb->OnAfterDeserialize();
    }

    // Phase 2: resolve pointer references
    for (DObject* obj : restoredObjects)
    {
        DELTA_VERIFY_MSG(obj != nullptr, "ObjectSnapshotReader restored object list contains null entry");
        VisitUnresolvedObjectReferencesInStruct(obj->GetClass(), obj,
            [&](DObjectPtrPropertyBase* ptrProp, void* valueAddress, const ScriptPointer& sp)
            {
                DELTA_VERIFY_MSG(ptrProp != nullptr, "ObjectSnapshotReader reference traversal produced null property");
                DELTA_VERIFY_MSG(valueAddress != nullptr, "ObjectSnapshotReader reference traversal produced null value address");

                DObject* resolved = nullptr;

                if (capturedSet.contains(sp.m_objectId))
                {
                    auto it = idMap.find(sp.m_objectId);
                    if (it != idMap.end())
                        resolved = it->second;
                    else if (!sp.IsNull())
                    {
                        DLOG(LogSerialization, ELogLevel::Warning,
                             "Failed to resolve captured snapshot reference '{}': id not present in restored object map (expected captured object)",
                             sp.m_objectId.ToString());
                    }
                }
                else if (db)
                {
                    resolved = db->FindObject(sp.m_assetId, sp.m_objectId);
                }
                else if (!sp.IsNull())
                {
                    DLOG(LogSerialization, ELogLevel::Warning,
                         "Failed to resolve external snapshot reference '{}:{}': asset database is null (expected database for external refs)",
                         sp.m_assetId.ToString(), sp.m_objectId.ToString());
                }

                ptrProp->ResolvePointer(valueAddress, resolved);
            });

        ResolveUnresolvedDelegateBindingsInStruct(obj->GetClass(), obj,
            [&](const ScriptPointer& sp) -> DObject*
            {
                if (capturedSet.contains(sp.m_objectId))
                {
                    auto idIt = idMap.find(sp.m_objectId);
                    if (idIt != idMap.end())
                        return idIt->second;
                    return nullptr;
                }
                if (db)
                    return db->FindObject(sp.m_assetId, sp.m_objectId);
                return nullptr;
            });
    }

    // Phase 3: reassemble object graph and post-restore
    DObject* root = restoredObjects.empty() ? nullptr : restoredObjects[0];

    if (auto* go = dynamic_cast<GameObject*>(root))
    {
        for (DComponent* comp : go->GetComponents())
        {
            if (comp)
            {
                comp->RegisterComponent(go);
            }
            else
            {
                DLOG(LogSerialization, ELogLevel::Warning,
                     "Skipped null restored component while registering GameObject '{}' (expected valid DComponent)",
                     go->GetName());
            }
        }

        for (SceneComponent* sc : go->GetSceneComponents())
        {
            if (sc)
            {
                sc->RegisterComponent(go);
            }
            else
            {
                DLOG(LogSerialization, ELogLevel::Warning,
                     "Skipped null restored scene component while registering GameObject '{}' (expected valid SceneComponent)",
                     go->GetName());
            }
        }

        if (world)
            world->AddGameObjectFromScene(go);
        if (addRestoredRootGameObjectToScene)
            addRestoredRootGameObjectToScene->AddGameObject(go);
    }

    // PostRestore bottom-up (leaves first)
    for (auto it = restoredObjects.rbegin(); it != restoredObjects.rend(); ++it)
    {
        DELTA_VERIFY_MSG(*it != nullptr, "ObjectSnapshotReader restored object list contains null entry");
        (*it)->PostRestore();
    }

    return root;
}
