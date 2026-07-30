#include "Runtime/Serialization/ObjectSnapshotWriter.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Serialization/ISerializationCallbackReceiver.h"
#include "Runtime/Serialization/JsonAssetArchive.h"
#include "Runtime/Logging/LogChannels.h"

#include <queue>

using namespace DeltaEngine;

void ObjectSnapshotWriter::CollectObjects(DObject* root, std::vector<DObject*>& out)
{
    DELTA_VERIFY_MSG(root != nullptr, "ObjectSnapshotWriter::CollectObjects requires a root object");

    if (auto* go = dynamic_cast<GameObject*>(root))
    {
        out.push_back(go);

        for (DComponent* comp : go->GetComponents())
        {
            if (comp)
            {
                out.push_back(comp);
            }
            else
            {
                DLOG(LogSerialization, ELogLevel::Warning,
                     "Skipped null component while capturing GameObject snapshot '{}' (expected valid DComponent pointer)",
                     go->GetName());
            }
        }

        std::queue<SceneComponent*> q;
        for (SceneComponent* sc : go->GetSceneComponents())
        {
            if (sc)
            {
                q.push(sc);
            }
            else
            {
                DLOG(LogSerialization, ELogLevel::Warning,
                     "Skipped null scene component while capturing GameObject snapshot '{}' (expected valid SceneComponent pointer)",
                     go->GetName());
            }
        }

        while (!q.empty())
        {
            SceneComponent* sc = q.front();
            q.pop();
            if (!sc)
            {
                DLOG(LogSerialization, ELogLevel::Warning,
                     "Skipped null scene component while traversing snapshot children (expected valid SceneComponent pointer)");
                continue;
            }
            out.push_back(sc);
            for (SceneComponent* child : sc->GetChildren())
            {
                if (child)
                {
                    q.push(child);
                }
                else
                {
                    DLOG(LogSerialization, ELogLevel::Warning,
                         "Skipped null scene component child while capturing snapshot for '{}' (expected valid child pointer)",
                         sc->GetName());
                }
            }
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
            if (!cur)
            {
                DLOG(LogSerialization, ELogLevel::Warning,
                     "Skipped null scene component while capturing scene component subtree (expected valid SceneComponent pointer)");
                continue;
            }
            out.push_back(cur);
            for (SceneComponent* child : cur->GetChildren())
            {
                if (child)
                {
                    q.push(child);
                }
                else
                {
                    DLOG(LogSerialization, ELogLevel::Warning,
                         "Skipped null scene component child while capturing snapshot for '{}' (expected valid child pointer)",
                         cur->GetName());
                }
            }
        }
        return;
    }

    out.push_back(root);
}

ObjectSnapshot ObjectSnapshotWriter::Capture(DObject* root)
{
    ObjectSnapshot snapshot;
    if (!root)
    {
        DLOG(LogSerialization, ELogLevel::Warning,
             "Skipped object snapshot capture: root object is null (expected valid DObject)");
        DELTA_ENSURE_MSG(false, "ObjectSnapshotWriter::Capture received null root");
        return snapshot;
    }

    DClass* rootClass = root->GetClass();
    DELTA_VERIFY_MSG(rootClass != nullptr,
                     "ObjectSnapshotWriter::Capture root object '{}' has no reflected class",
                     root->GetObjectId().ToString());
    snapshot.rootClassName = rootClass ? rootClass->GetName() : std::string{};

    std::vector<DObject*> objects;
    CollectObjects(root, objects);

    for (DObject* obj : objects)
    {
        DELTA_VERIFY_MSG(obj != nullptr, "ObjectSnapshotWriter collected a null object");
        snapshot.capturedIds.push_back(obj->GetObjectId());
    }

    JsonAssetArchive ar;
    ar.BeginArray("objects", objects.size());

    for (DObject* obj : objects)
    {
        DELTA_VERIFY_MSG(obj != nullptr, "ObjectSnapshotWriter collected a null object");
        if (auto* cb = dynamic_cast<ISerializationCallbackReceiver*>(obj))
            cb->OnBeforeSerialize();

        DClass* cls = obj->GetClass();
        DELTA_VERIFY_MSG(cls != nullptr,
                         "ObjectSnapshotWriter cannot serialize object '{}' because reflected class is null",
                         obj->GetObjectId().ToString());
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
