#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>
#include <memory>
#include <vector>

#include "SimpleMath.h"
#include "Runtime/Core/DObject.h"

DELTA_ENGINE_NS_BEGIN

class DWorld;

class SceneComponent : public DObject, public std::enable_shared_from_this<SceneComponent>
{
    friend class DWorld;

public:
    SceneComponent();

    DirectX::SimpleMath::Vector3 GetLocalPosition() const;
    DirectX::SimpleMath::Vector3 GetWorldPosition() const;
    /// World transform matrix for rendering (model matrix). Updated when hierarchy changes.
    inline DirectX::XMMATRIX GetWorldTransform() const { return m_worldTransform; }
    void SetLocalPosition(DirectX::SimpleMath::Vector3 position);
    void SetLocalPosition(DirectX::XMVECTOR position);
    void SetLocalPosition(float x, float y, float z);
    void SetWorldPosition(DirectX::SimpleMath::Vector3 position);
    void SetWorldPosition(DirectX::XMVECTOR position);
    void SetWorldPosition(float x, float y, float z);

    DirectX::SimpleMath::Quaternion GetLocalRotation() const;
    DirectX::SimpleMath::Quaternion GetWorldRotation() const;
    void SetLocalRotation(DirectX::SimpleMath::Quaternion rotation);
    void SetLocalRotation(DirectX::XMVECTOR rotation);
    void SetWorldRotation(DirectX::SimpleMath::Quaternion rotation);
    void SetWorldRotation(DirectX::XMVECTOR rotation);

    DirectX::SimpleMath::Vector3 GetLocalScale() const;
    DirectX::SimpleMath::Vector3 GetWorldScale() const;
    void SetLocalScale(DirectX::SimpleMath::Vector3 scale);
    void SetLocalScale(float x, float y, float z);

    DirectX::SimpleMath::Vector3 GetRight() const;
    DirectX::SimpleMath::Vector3 GetUp() const;
    DirectX::SimpleMath::Vector3 GetForward() const;
    
    std::shared_ptr<SceneComponent> GetParent() const { return m_parent.lock(); }
    void SetParent(std::shared_ptr<SceneComponent> parent);

protected:
    virtual void OnTransformChanged() {}

private:
    DirectX::XMMATRIX m_localTransform;
    DirectX::XMMATRIX m_worldTransform;
    std::weak_ptr<SceneComponent> m_parent;
    std::vector<std::shared_ptr<SceneComponent>> m_children;

    void UpdateTransformHierarchy(DirectX::XMMATRIX worldTransform);
    void UpdateTransform();
    void SetTransformDirty();
};

template<typename T>
concept IsSceneComponent = std::derived_from<T, SceneComponent>;

DELTA_ENGINE_NS_END