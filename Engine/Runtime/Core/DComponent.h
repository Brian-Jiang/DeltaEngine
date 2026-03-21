#pragma once

#include "EngineIncludes.h"

#include <string>
#include <type_traits>

#include "Core/DObject.h"

#include "DComponent.generated.h"

DELTA_ENGINE_NS_BEGIN

class GameObject;

DCLASS()
class DComponent: public DObject
{
    DGENERATED_BODY(DComponent)

public:
    DELTAENGINE_API DComponent();
    DELTAENGINE_API virtual ~DComponent();

    /// Registers the owning game object for this component.
    virtual void RegisterComponent(GameObject* gameObject);
    /// Removes the component from its owner and asset.
    void MarkForDestroy();

public:
    /// Sets the component display name.
    DFUNCTION()
    DELTAENGINE_API void SetName(const std::string& name);

    /// Returns the owning game object.
    DFUNCTION()
    DELTAENGINE_API GameObject* GetGameObject() const;

    /// Returns the component display name.
    DFUNCTION()
    DELTAENGINE_API const std::string& GetName() const;

private:
    GameObject* m_gameObject;

    DPROPERTY()
    std::string m_name;
};

template<typename T>
concept IsDComponent = std::derived_from<T, DComponent>;

DELTA_ENGINE_NS_END
