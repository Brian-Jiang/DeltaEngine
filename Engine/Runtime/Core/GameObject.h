#pragma once

#include "EngineIncludes.h"

#include <vector>
#include <memory>
#include <string>

#include "Core/DObject.h"
#include "Core/DComponent.h"
#include "Core/SceneComponent.h"
#include "Core/DWorld.h"

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
	//DELTAENGINE_API GameObject(const std::string& name);
	DELTAENGINE_API ~GameObject();

	template <typename T> requires IsDComponent<T>
	T* AddComponent(std::string name = "New Component")
    {
        T* component = CreateDObject<T>();
        component->RegisterComponent(this);
        component->SetName(name);
        m_components.push_back(component);
        return component;
	}

	template <typename T> requires IsSceneComponent<T>
    T* AddSceneComponent(std::string name = "New Scene Component")
    {
        T* sceneComponent = CreateDObject<T>();  // alignment issue
        sceneComponent->RegisterComponent(this);
        sceneComponent->SetName(name);
        m_sceneComponents.push_back(sceneComponent);
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

    DELTAENGINE_API DComponent* AddComponentByClass(const DClass* dclass);

    DELTAENGINE_API void RemoveComponent(DComponent* component);

    DFUNCTION()
    DELTAENGINE_API void Destroy();

    DFUNCTION()
    DELTAENGINE_API const std::string& GetName() const;

    DFUNCTION()
    DELTAENGINE_API DWorld* GetCurrentWorld() const;
    DFUNCTION()
    DELTAENGINE_API SceneComponent* GetRootSceneComponent() const;
    DFUNCTION()
    DELTAENGINE_API const std::vector<SceneComponent*>& GetSceneComponents() const;
    DFUNCTION()
    DELTAENGINE_API const std::vector<DComponent*>& GetComponents() const;

    template <typename T> requires IsSceneComponent<T>
    DELTAENGINE_API inline T* GetRootSceneComponent() const
    {
        return static_cast<T*>(m_rootSceneComponent); 
    }

private:
    DPROPERTY()
    std::string m_name;

    SceneComponent* m_rootSceneComponent;
    std::vector<SceneComponent*> m_sceneComponents;
    std::vector<DComponent*> m_components;
    DWorld* m_currentWorld;
};

DELTA_ENGINE_NS_END
