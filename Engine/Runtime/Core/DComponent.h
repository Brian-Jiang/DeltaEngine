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
    DComponent(std::string name) = delete;

    DELTAENGINE_API void Initialize(std::string name);

    DComponent(std::string name, std::shared_ptr<GameObject> gameObject) = delete;

    DELTAENGINE_API void Initialize(std::string name, std::shared_ptr<GameObject> gameObject);

    DELTAENGINE_API virtual ~DComponent();

public:
    DFUNCTION()
    std::shared_ptr<GameObject> GetGameObject() const;

    DFUNCTION()
    const std::string& GetName() const;

private:
    std::weak_ptr<GameObject> m_gameObject;

    DPROPERTY()
    std::string m_name;
};

template<typename T>
concept IsDComponent = std::derived_from<T, DComponent>;

DELTA_ENGINE_NS_END