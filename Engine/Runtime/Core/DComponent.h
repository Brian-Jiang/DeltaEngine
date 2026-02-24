#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <string>
#include <type_traits>

#include "Core/DObject.h"

DELTA_ENGINE_NS_BEGIN

class GameObject;

class DComponent: public DObject
{

public:
    DELTAENGINE_API DComponent();
    DELTAENGINE_API DComponent(std::string name);
    DELTAENGINE_API DComponent(std::string name, std::shared_ptr<GameObject> gameObject);
    DELTAENGINE_API virtual ~DComponent();

public:
    inline std::shared_ptr<GameObject> GetGameObject() const { return m_gameObject.lock(); }
    inline const std::string& GetName() const { return m_name; }

private:
    std::weak_ptr<GameObject> m_gameObject;
    std::string m_name;
};

template<typename T>
concept IsDComponent = std::derived_from<T, DComponent>;

DELTA_ENGINE_NS_END