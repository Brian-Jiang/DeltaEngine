#include "Serialization/ObjectSnapshotReader.h"

#include "Assets/IAssetDatabase.h"
#include "Core/DObject.h"
#include "Core/DComponent.h"
#include "Core/GameObject.h"
#include "Core/SceneComponent.h"
#include "Core/DWorld.h"
#include "Assets/DPrimaryAsset.h"
#include "Core/DScene.h"
#include "Reflection/DClass.h"
#include "Reflection/DObjectReferenceTraversal.h"
#include "Reflection/ReflectionRegistry.h"
#include "Serialization/ISerializationCallbackReceiver.h"
#include "Serialization/JsonAssetArchive.h"
#include "Serialization/ObjectSnapshot.h"

#include <algorithm>
#include <cstdio>
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
        return nullptr;

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
            ar.EndObject();
            continue;
        }

        DObject* obj = GetReflectionRegistry().CreateObject(className);
        if (!obj)
        {
            ar.EndObject();
            continue;
        }

        ObjectId oid;
        ar.Serialize("_objectId", oid);

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
            std::printf("ObjectSnapshotReader: objectId collision for %s, remapping %s -> %s\n",
                        className.c_str(), oid.ToString().c_str(), fresh.ToString().c_str());
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
            if (obj && !obj->HasOwningAsset())
                registerWithAsset->AddObject(obj);
        }
    }

    // OnAfterDeserialize
    for (DObject* obj : restoredObjects)
    {
        if (auto* cb = dynamic_cast<ISerializationCallbackReceiver*>(obj))
            cb->OnAfterDeserialize();
    }

    // Phase 2: resolve pointer references
    for (DObject* obj : restoredObjects)
    {
        VisitUnresolvedObjectReferencesInStruct(obj->GetClass(), obj,
            [&](DObjectPtrPropertyBase* ptrProp, void* valueAddress, const ScriptPointer& sp)
            {
                DObject* resolved = nullptr;

                if (capturedSet.contains(sp.m_objectId))
                {
                    auto it = idMap.find(sp.m_objectId);
                    if (it != idMap.end())
                        resolved = it->second;
                }
                else if (db)
                {
                    resolved = db->FindObject(sp.m_assetId, sp.m_objectId);
                }

                ptrProp->ResolvePointer(valueAddress, resolved);
            });
    }

    // Phase 3: reassemble object graph and post-restore
    DObject* root = restoredObjects.empty() ? nullptr : restoredObjects[0];

    if (auto* go = dynamic_cast<GameObject*>(root))
    {
        for (DComponent* comp : go->GetComponents())
            comp->RegisterComponent(go);

        for (SceneComponent* sc : go->GetSceneComponents())
            sc->RegisterComponent(go);

        if (world)
            world->AddGameObjectFromScene(go);
        if (addRestoredRootGameObjectToScene)
            addRestoredRootGameObjectToScene->AddGameObject(go);
    }

    // PostRestore bottom-up (leaves first)
    for (auto it = restoredObjects.rbegin(); it != restoredObjects.rend(); ++it)
        (*it)->PostRestore();

    return root;
}
