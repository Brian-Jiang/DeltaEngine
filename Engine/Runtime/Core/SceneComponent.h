#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>
#include <memory>
#include <vector>

#include "SimpleMath.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Core/DComponent.h"

#include "SceneComponent.generated.h"

DELTA_ENGINE_NS_BEGIN

class DWorld;

DCLASS()
class SceneComponent : public DComponent, public std::enable_shared_from_this<SceneComponent>
{
    DGENERATED_BODY(SceneComponent)
    friend class DWorld;

public:
    DELTAENGINE_API SceneComponent();
    //DELTAENGINE_API SceneComponent(std::string name);
    //DELTAENGINE_API SceneComponent(std::string name, std::shared_ptr<GameObject> gameObject);

    DFUNCTION()
    DELTAENGINE_API DirectX::SimpleMath::Vector3 GetLocalPosition() const;
    DFUNCTION()
    DELTAENGINE_API DirectX::SimpleMath::Vector3 GetWorldPosition() const;
    /// World transform matrix for rendering (model matrix). Updated when hierarchy changes.
    DFUNCTION()
    DELTAENGINE_API DirectX::XMMATRIX GetWorldTransform() const;
    DFUNCTION()
    DELTAENGINE_API void SetLocalPosition(DirectX::SimpleMath::Vector3 position);
    DFUNCTION()
    DELTAENGINE_API void SetLocalPosition(DirectX::XMVECTOR position);
    DFUNCTION()
    DELTAENGINE_API void SetLocalPosition(float x, float y, float z);
    DFUNCTION()
    DELTAENGINE_API void SetWorldPosition(DirectX::SimpleMath::Vector3 position);
    DFUNCTION()
    DELTAENGINE_API void SetWorldPosition(DirectX::XMVECTOR position);
    DFUNCTION()
    DELTAENGINE_API void SetWorldPosition(float x, float y, float z);

    DFUNCTION()
    DELTAENGINE_API DirectX::SimpleMath::Quaternion GetLocalRotation() const;
    DFUNCTION()
    DELTAENGINE_API DirectX::SimpleMath::Vector3 GetLocalRotationEulerAngles() const;
    DFUNCTION()
    DELTAENGINE_API DirectX::SimpleMath::Quaternion GetWorldRotation() const;
    DFUNCTION()
    DELTAENGINE_API void SetLocalRotation(DirectX::SimpleMath::Quaternion rotation);
    DFUNCTION()
    DELTAENGINE_API void SetLocalRotation(DirectX::XMVECTOR rotation);
    DFUNCTION()
    DELTAENGINE_API void SetLocalRotation(DirectX::SimpleMath::Vector3 eulerAngles);
    DFUNCTION()
    DELTAENGINE_API void SetWorldRotation(DirectX::SimpleMath::Quaternion rotation);
    DFUNCTION()
    DELTAENGINE_API void SetWorldRotation(DirectX::XMVECTOR rotation);

    DFUNCTION()
    DELTAENGINE_API DirectX::SimpleMath::Vector3 GetLocalScale() const;
    DFUNCTION()
    DELTAENGINE_API DirectX::SimpleMath::Vector3 GetWorldScale() const;
    DFUNCTION()
    DELTAENGINE_API void SetLocalScale(DirectX::SimpleMath::Vector3 scale);
    DFUNCTION()
    DELTAENGINE_API void SetLocalScale(float x, float y, float z);

    DFUNCTION()
    DELTAENGINE_API DirectX::SimpleMath::Vector3 GetRight() const;
    DFUNCTION()
    DELTAENGINE_API DirectX::SimpleMath::Vector3 GetUp() const;
    DFUNCTION()
    DELTAENGINE_API DirectX::SimpleMath::Vector3 GetForward() const;
    
    DFUNCTION()
    DELTAENGINE_API std::shared_ptr<SceneComponent> GetParent() const;
    DFUNCTION()
    DELTAENGINE_API const std::vector<std::shared_ptr<SceneComponent>>& GetChildren() const;
    DFUNCTION()
    DELTAENGINE_API void SetParent(std::shared_ptr<SceneComponent> parent);

protected:
    virtual void OnTransformChanged() {}

private:
    DPROPERTY()
    DirectX::XMMATRIX m_localTransform;
    DPROPERTY()
    DirectX::XMMATRIX m_worldTransform;
    DPROPERTY()
    std::weak_ptr<SceneComponent> m_parent;
    DPROPERTY()
    std::vector<std::shared_ptr<SceneComponent>> m_children;
    DPROPERTY()
    DirectX::SimpleMath::Vector3 m_eulerRotationCache;

    void UpdateTransformHierarchy(DirectX::XMMATRIX worldTransform);
    void UpdateTransform();
    void SetTransformDirty();
};

template<typename T>
concept IsSceneComponent = std::derived_from<T, SceneComponent>;

DELTA_ENGINE_NS_END