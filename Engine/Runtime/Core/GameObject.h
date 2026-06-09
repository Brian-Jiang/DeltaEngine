#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Core/DComponent.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Assets/DPrimaryAsset.h"

#include <string>
#include <vector>

#include "GameObject.generated.h"

DELTA_ENGINE_NS_BEGIN

class DClass;

DCLASS()
class GameObject: public DObject
{
    DGENERATED_BODY(GameObject)
    friend class DWorld;

public:
	DELTAENGINE_API GameObject();
	DELTAENGINE_API ~GameObject();

    /// Creates and attaches a component of the requested type.
	template <typename T> requires IsDComponent<T>
	T* AddComponent(std::string name = "New Component")
    {
        T* component = CreateDObject<T>();
        if (HasOwningAsset())
        {
            GetOwningAsset()->AddObject(component);
        }
        
        component->RegisterComponent(this);
        component->SetName(name);
        m_components.push_back(component);
        return component;
	}

    /// Creates and attaches a scene component of the requested type.
	template <typename T> requires IsSceneComponent<T>
    T* AddSceneComponent(std::string name = "New Scene Component")
    {
        T* sceneComponent = CreateDObject<T>();
        if (HasOwningAsset())
        {
            GetOwningAsset()->AddObject(sceneComponent);
        }

        sceneComponent->RegisterComponent(this);
        sceneComponent->SetName(name);
        m_sceneComponents.push_back(sceneComponent);
        DELTA_ASSERT(m_currentWorld != nullptr);
        if (!m_rootSceneComponent)
        {
            m_rootSceneComponent = sceneComponent;
            auto worldSceneRoot = m_currentWorld->GetRootSceneComponent();
            if (worldSceneRoot)
            {
                sceneComponent->SetParent(worldSceneRoot);
            }
        }
        else
        {
            sceneComponent->SetParent(m_rootSceneComponent);
        }

        return sceneComponent;
    }

    /// Creates and attaches a reflected component instance.
    DELTAENGINE_API DComponent* AddComponentByClass(const DClass* dclass);

    /// Unlinks a component from this game object (GC reclaims later).
    DELTAENGINE_API void RemoveComponent(DComponent* component);

    /// Removes a component from this game object's lists without destroying it.
    DELTAENGINE_API void DetachComponent(DComponent* component);

    /// Inserts a pre-allocated regular component at the given index.
    DELTAENGINE_API void InsertComponent(DComponent* comp, int index);

    /// Inserts a pre-allocated scene component at the given index with optional parent.
    DELTAENGINE_API void InsertSceneComponent(SceneComponent* sc, int index, SceneComponent* parent);

    /// Unlinks this game object and all owned components.
    DFUNCTION()
    DELTAENGINE_API void Destroy();

    /// Returns the game object display name.
    DFUNCTION()
    DELTAENGINE_API const std::string& GetName() const;

    /// Returns the world that currently owns this game object.
    DFUNCTION()
    DELTAENGINE_API DWorld* GetCurrentWorld() const;
    /// Returns the root scene component, if one exists.
    DFUNCTION()
    DELTAENGINE_API SceneComponent* GetRootSceneComponent() const;
    /// Returns all attached scene components.
    DFUNCTION()
    DELTAENGINE_API const std::vector<SceneComponent*>& GetSceneComponents() const;
    /// Returns all attached non-scene components.
    DFUNCTION()
    DELTAENGINE_API const std::vector<DComponent*>& GetComponents() const;

    template <typename T> requires IsSceneComponent<T>
    DELTAENGINE_API inline T* GetRootSceneComponent() const
    {
        return dynamic_cast<T*>(m_rootSceneComponent); 
    }

private:
    DPROPERTY()
    std::string m_name;

    DPROPERTY(HideInDetails)
    SceneComponent* m_rootSceneComponent;

    DPROPERTY(HideInDetails)
    std::vector<SceneComponent*> m_sceneComponents;

    DPROPERTY(HideInDetails)
    std::vector<DComponent*> m_components;

    DWorld* m_currentWorld;
};

DELTA_ENGINE_NS_END
