#pragma once

#include "EngineIncludes.h"

#include <memory>
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
    //DELTAENGINE_API DComponent(std::string name);
    //DELTAENGINE_API DComponent(std::string name, std::shared_ptr<GameObject> gameObject);
    DELTAENGINE_API virtual ~DComponent();

    virtual void RegisterComponent(std::shared_ptr<GameObject> gameObject);

public:
    DFUNCTION()
    DELTAENGINE_API void SetName(const std::string& name);

    DFUNCTION()
    DELTAENGINE_API std::shared_ptr<GameObject> GetGameObject() const;

    DFUNCTION()
    DELTAENGINE_API const std::string& GetName() const;

private:
    std::weak_ptr<GameObject> m_gameObject;

    DPROPERTY()
    std::string m_name;
};

template<typename T>
concept IsDComponent = std::derived_from<T, DComponent>;

DELTA_ENGINE_NS_END