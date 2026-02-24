#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>
#include <memory>
#include <vector>

#include "SimpleMath.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Core/DComponent.h"

DELTA_ENGINE_NS_BEGIN

class DWorld;

class SceneComponent : public DComponent, public std::enable_shared_from_this<SceneComponent>
{
    friend class DWorld;

public:
    DELTAENGINE_API SceneComponent();
    DELTAENGINE_API SceneComponent(std::string name);
    DELTAENGINE_API SceneComponent(std::string name, std::shared_ptr<GameObject> gameObject);

    DELTAENGINE_API DirectX::SimpleMath::Vector3 GetLocalPosition() const;
    DELTAENGINE_API DirectX::SimpleMath::Vector3 GetWorldPosition() const;
    /// World transform matrix for rendering (model matrix). Updated when hierarchy changes.
    inline DirectX::XMMATRIX GetWorldTransform() const { return m_worldTransform; }
    DELTAENGINE_API void SetLocalPosition(DirectX::SimpleMath::Vector3 position);
    DELTAENGINE_API void SetLocalPosition(DirectX::XMVECTOR position);
    DELTAENGINE_API void SetLocalPosition(float x, float y, float z);
    DELTAENGINE_API void SetWorldPosition(DirectX::SimpleMath::Vector3 position);
    DELTAENGINE_API void SetWorldPosition(DirectX::XMVECTOR position);
    DELTAENGINE_API void SetWorldPosition(float x, float y, float z);

    DELTAENGINE_API DirectX::SimpleMath::Quaternion GetLocalRotation() const;
    DELTAENGINE_API DirectX::SimpleMath::Vector3 GetLocalRotationEulerAngles() const;
    DELTAENGINE_API DirectX::SimpleMath::Quaternion GetWorldRotation() const;
    DELTAENGINE_API void SetLocalRotation(DirectX::SimpleMath::Quaternion rotation);
    DELTAENGINE_API void SetLocalRotation(DirectX::XMVECTOR rotation);
    DELTAENGINE_API void SetLocalRotation(DirectX::SimpleMath::Vector3 eulerAngles);
    DELTAENGINE_API void SetWorldRotation(DirectX::SimpleMath::Quaternion rotation);
    DELTAENGINE_API void SetWorldRotation(DirectX::XMVECTOR rotation);

    DELTAENGINE_API DirectX::SimpleMath::Vector3 GetLocalScale() const;
    DELTAENGINE_API DirectX::SimpleMath::Vector3 GetWorldScale() const;
    DELTAENGINE_API void SetLocalScale(DirectX::SimpleMath::Vector3 scale);
    DELTAENGINE_API void SetLocalScale(float x, float y, float z);

    DELTAENGINE_API DirectX::SimpleMath::Vector3 GetRight() const;
    DELTAENGINE_API DirectX::SimpleMath::Vector3 GetUp() const;
    DELTAENGINE_API DirectX::SimpleMath::Vector3 GetForward() const;
    
    DELTAENGINE_API std::shared_ptr<SceneComponent> GetParent() const { return m_parent.lock(); }
    DELTAENGINE_API const std::vector<std::shared_ptr<SceneComponent>>& GetChildren() const { return m_children; }
    DELTAENGINE_API void SetParent(std::shared_ptr<SceneComponent> parent);

protected:
    virtual void OnTransformChanged() {}

private:
    DirectX::XMMATRIX m_localTransform;
    DirectX::XMMATRIX m_worldTransform;
    std::weak_ptr<SceneComponent> m_parent;
    std::vector<std::shared_ptr<SceneComponent>> m_children;
    DirectX::SimpleMath::Vector3 m_eulerRotationCache;

    void UpdateTransformHierarchy(DirectX::XMMATRIX worldTransform);
    void UpdateTransform();
    void SetTransformDirty();
};

template<typename T>
concept IsSceneComponent = std::derived_from<T, SceneComponent>;

DELTA_ENGINE_NS_END