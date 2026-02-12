#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <type_traits>

#include "Core/DObject.h"

DELTA_ENGINE_NS_BEGIN

class GameObject;

class DComponent: public DObject
{

public:
    DComponent();
    DComponent(std::shared_ptr<GameObject> gameObject);
    ~DComponent();

public:
    inline std::shared_ptr<GameObject> GetGameObject() const { return m_gameObject; }

private:
    std::shared_ptr<GameObject> m_gameObject;

};

template<typename T>
concept IsDComponent = std::is_base_of_v<DComponent, T>;

DELTA_ENGINE_NS_END