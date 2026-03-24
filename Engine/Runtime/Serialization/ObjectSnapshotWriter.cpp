#include "Serialization/ObjectSnapshotWriter.h"

#include "Core/DObject.h"
#include "Core/GameObject.h"
#include "Core/SceneComponent.h"
#include "Reflection/DClass.h"
#include "Serialization/JsonAssetArchive.h"
#include "Serialization/ISerializationCallbackReceiver.h"

#include <queue>

using namespace DeltaEngine;

void ObjectSnapshotWriter::CollectObjects(DObject* root, std::vector<DObject*>& out)
{
    if (!root)
        return;

    if (auto* go = dynamic_cast<GameObject*>(root))
    {
        out.push_back(go);

        for (DComponent* comp : go->GetComponents())
            out.push_back(comp);

        std::queue<SceneComponent*> q;
        for (SceneComponent* sc : go->GetSceneComponents())
            q.push(sc);

        while (!q.empty())
        {
            SceneComponent* sc = q.front();
            q.pop();
            out.push_back(sc);
            for (SceneComponent* child : sc->GetChildren())
                q.push(child);
        }
        return;
    }

    if (auto* sc = dynamic_cast<SceneComponent*>(root))
    {
        std::queue<SceneComponent*> q;
        q.push(sc);
        while (!q.empty())
        {
            SceneComponent* cur = q.front();
            q.pop();
            out.push_back(cur);
            for (SceneComponent* child : cur->GetChildren())
                q.push(child);
        }
        return;
    }

    out.push_back(root);
}

ObjectSnapshot ObjectSnapshotWriter::Capture(DObject* root)
{
    ObjectSnapshot snapshot;
    if (!root)
        return snapshot;

    DClass* rootClass = root->GetClass();
    snapshot.rootClassName = rootClass ? rootClass->GetName() : std::string{};

    std::vector<DObject*> objects;
    CollectObjects(root, objects);

    for (DObject* obj : objects)
        snapshot.capturedIds.push_back(obj->GetObjectId());

    JsonAssetArchive ar;
    ar.BeginArray("objects", objects.size());

    for (DObject* obj : objects)
    {
        if (auto* cb = dynamic_cast<ISerializationCallbackReceiver*>(obj))
            cb->OnBeforeSerialize();

        DClass* cls = obj->GetClass();
        std::string className = cls->GetName();
        ar.BeginObject(className);

        UUID oid = obj->GetObjectId();
        ar.Serialize("_objectId", oid);

        cls->Serialize(ar, *obj);

        ar.EndObject();
    }

    ar.EndArray();
    snapshot.rootJson = ar.GetRoot();
    return snapshot;
}
