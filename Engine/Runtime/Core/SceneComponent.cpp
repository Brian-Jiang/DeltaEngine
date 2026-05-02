#include "Runtime/Core/SceneComponent.h"

#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"

#include <cstdint>

using namespace DirectX;
using namespace DeltaEngine;

SceneComponent::SceneComponent()
    : DComponent()
    , m_localTransform(XMMatrixIdentity())
    , m_worldTransform(XMMatrixIdentity())
    , m_parent(nullptr)
{
}

SimpleMath::Vector3 SceneComponent::GetLocalPosition() const
{
    return SimpleMath::Vector3(m_localTransform.r[3]);
}

DirectX::SimpleMath::Vector3 DeltaEngine::SceneComponent::GetWorldPosition() const
{
    return SimpleMath::Vector3(m_worldTransform.r[3]);
}

void SceneComponent::SetLocalPosition(DirectX::SimpleMath::Vector3 position)
{
    m_localTransform.r[3] = XMVectorSet(position.x, position.y, position.z, 1.0f);
    SetTransformDirty();
}

void SceneComponent::SetLocalPosition(DirectX::XMVECTOR position)
{
    m_localTransform.r[3] = position;
    SetTransformDirty();
}

void SceneComponent::SetLocalPosition(float x, float y, float z)
{
    m_localTransform.r[3] = XMVectorSet(x, y, z, 1.0f);
    SetTransformDirty();
}

void SceneComponent::SetWorldPosition(DirectX::SimpleMath::Vector3 position)
{
    SetWorldPosition(position.x, position.y, position.z);
}

void SceneComponent::SetWorldPosition(DirectX::XMVECTOR position)
{
    SetWorldPosition(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position));
}

void SceneComponent::SetWorldPosition(float x, float y, float z)
{
    if (m_parent)
    {
        auto position = XMVectorSet(x, y, z, 1.0f);
        XMVECTOR parentPosition = m_parent->m_worldTransform.r[3];
        auto localPositionParentSpace = position - parentPosition;
        SetLocalPosition(localPositionParentSpace);
    }
    else
    {
        SetLocalPosition(x, y, z);
    }
}

DirectX::SimpleMath::Quaternion DeltaEngine::SceneComponent::GetLocalRotation() const
{
    XMVECTOR translation;
    XMVECTOR currentRotation;
    XMVECTOR scale;
    bool success = XMMatrixDecompose(&scale, &currentRotation, &translation, m_localTransform);
    if (!success)
    {
        DLOG(LogCore, ELogLevel::Warning,
            "SceneComponent::GetLocalRotation: XMMatrixDecompose failed on local transform (this={}) — returning identity quaternion",
            reinterpret_cast<uintptr_t>(this));
        return SimpleMath::Quaternion::Identity;
    }

    return SimpleMath::Quaternion(currentRotation);
}

DirectX::SimpleMath::Vector3 DeltaEngine::SceneComponent::GetLocalRotationEulerAngles() const
{
    return m_eulerRotationCache;
}

DirectX::SimpleMath::Quaternion DeltaEngine::SceneComponent::GetWorldRotation() const
{
    XMVECTOR translation;
    XMVECTOR currentRotation;
    XMVECTOR scale;
    bool success = XMMatrixDecompose(&scale, &currentRotation, &translation, m_worldTransform);
    if (!success)
    {
        DLOG(LogCore, ELogLevel::Warning,
            "SceneComponent::GetWorldRotation: XMMatrixDecompose failed on world transform (this={}) — returning identity quaternion",
            reinterpret_cast<uintptr_t>(this));
        return SimpleMath::Quaternion::Identity;
    }

    return SimpleMath::Quaternion(currentRotation);
}

void SceneComponent::SetLocalRotation(DirectX::SimpleMath::Quaternion rotation)
{
    auto eulerRadian = SimpleMath::Quaternion(rotation).ToEuler();
    m_eulerRotationCache = SimpleMath::Vector3(XMConvertToDegrees(eulerRadian.x), XMConvertToDegrees(eulerRadian.y), XMConvertToDegrees(eulerRadian.z));
    SetLocalRotation((XMVECTOR) rotation);
}

void SceneComponent::SetLocalRotation(DirectX::XMVECTOR rotation)
{
    XMVECTOR translation;
    XMVECTOR currentRotation;
    XMVECTOR scale;
    bool success = XMMatrixDecompose(&scale, &currentRotation, &translation, m_localTransform);
    if (!success)
    {
        DLOG(LogCore, ELogLevel::Warning,
            "SceneComponent::SetLocalRotation(XMVECTOR): XMMatrixDecompose failed on local transform (this={}) — no-op",
            reinterpret_cast<uintptr_t>(this));
        return;
    }

    m_localTransform = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rotation) * XMMatrixTranslationFromVector(translation);
    SetTransformDirty();
}

void DeltaEngine::SceneComponent::SetLocalRotation(DirectX::SimpleMath::Vector3 eulerAngles)
{
    m_eulerRotationCache = eulerAngles;
    XMVECTOR rotationQuat = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(eulerAngles.x), XMConvertToRadians(eulerAngles.y), XMConvertToRadians(eulerAngles.z));
    SetLocalRotation(rotationQuat);
}

void DeltaEngine::SceneComponent::SetWorldRotation(DirectX::SimpleMath::Quaternion rotation)
{
    SetWorldRotation((XMVECTOR) rotation);
}

void DeltaEngine::SceneComponent::SetWorldRotation(DirectX::XMVECTOR rotation)
{
    if (m_parent)
    {
        auto parentWorldTransform = m_parent->m_worldTransform;
        XMVECTOR translation;
        XMVECTOR parentRotation;
        XMVECTOR scale;
        bool success = XMMatrixDecompose(&scale, &parentRotation, &translation, parentWorldTransform);
        if (!success)
        {
            DLOG(LogCore, ELogLevel::Warning,
                "SceneComponent::SetWorldRotation: XMMatrixDecompose failed on parent world transform (this={}, parent={}) — no-op",
                reinterpret_cast<uintptr_t>(this), reinterpret_cast<uintptr_t>(m_parent));
            return;
        }

        XMVECTOR parentInverseRotation = XMQuaternionInverse(parentRotation);
        auto localRotationParentSpace = XMQuaternionMultiply(rotation, parentInverseRotation);
        auto eulerRadian = SimpleMath::Quaternion(localRotationParentSpace).ToEuler();
        m_eulerRotationCache = SimpleMath::Vector3(XMConvertToDegrees(eulerRadian.x), XMConvertToDegrees(eulerRadian.y), XMConvertToDegrees(eulerRadian.z));
        SetLocalRotation(localRotationParentSpace);
    }
    else
    {
        SetLocalRotation(rotation);
    }
}

SimpleMath::Vector3 DeltaEngine::SceneComponent::GetLocalScale() const {
    XMVECTOR translation;
    XMVECTOR rotation;
    XMVECTOR scale;
    bool success = XMMatrixDecompose(&scale, &rotation, &translation, m_localTransform);
    if (!success)
    {
        DLOG(LogCore, ELogLevel::Warning,
            "SceneComponent::GetLocalScale: XMMatrixDecompose failed on local transform (this={}) — returning Ones",
            reinterpret_cast<uintptr_t>(this));
        return SimpleMath::Vector3::One;
    }

    return SimpleMath::Vector3(scale);
}

DirectX::SimpleMath::Vector3 DeltaEngine::SceneComponent::GetWorldScale() const {
    XMVECTOR translation;
    XMVECTOR rotation;
    XMVECTOR scale;
    bool success = XMMatrixDecompose(&scale, &rotation, &translation, m_worldTransform);
    if (!success)
    {
        DLOG(LogCore, ELogLevel::Warning,
            "SceneComponent::GetWorldScale: XMMatrixDecompose failed on world transform (this={}) — returning Ones",
            reinterpret_cast<uintptr_t>(this));
        return SimpleMath::Vector3::One;
    }

    return SimpleMath::Vector3(scale);
}

void DeltaEngine::SceneComponent::SetLocalScale(DirectX::SimpleMath::Vector3 scale) {
    SetLocalScale(scale.x, scale.y, scale.z);
}

void DeltaEngine::SceneComponent::SetLocalScale(float x, float y, float z) {
    XMVECTOR scale = XMVectorSet(x, y, z, 1.0);
    XMVECTOR translation;
    XMVECTOR rotation;
    XMVECTOR currentScale;
    bool success = XMMatrixDecompose(&currentScale, &rotation, &translation, m_localTransform);
    if (!success)
    {
        DLOG(LogCore, ELogLevel::Warning,
            "SceneComponent::SetLocalScale: XMMatrixDecompose failed on local transform (this={}) — no-op",
            reinterpret_cast<uintptr_t>(this));
        return;
    }

    m_localTransform = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rotation) * XMMatrixTranslationFromVector(translation);
    SetTransformDirty();
}

DirectX::SimpleMath::Vector3 DeltaEngine::SceneComponent::GetRight() const
{
    return m_worldTransform.r[0];
}

DirectX::SimpleMath::Vector3 DeltaEngine::SceneComponent::GetUp() const
{
    return m_worldTransform.r[1];
}

DirectX::SimpleMath::Vector3 DeltaEngine::SceneComponent::GetForward() const
{
    return m_worldTransform.r[2];
}

XMMATRIX DeltaEngine::SceneComponent::GetWorldTransform() const { return m_worldTransform; }

SceneComponent* DeltaEngine::SceneComponent::GetParent() const { return m_parent; }

const std::vector<SceneComponent*>& DeltaEngine::SceneComponent::GetChildren() const { return m_children; }

void DeltaEngine::SceneComponent::SetParent(SceneComponent* parent)
{
    if (parent == m_parent)
    {
        return;
    }

    if (parent != nullptr)
    {
        // Check for circular reference
        SceneComponent* current = parent;
        while (current) {
            if (current == this)
            {
                DLOG(LogCore, ELogLevel::Warning,
                    "SceneComponent::SetParent: rejected circular parent assignment (this={}, proposedParent={})",
                    reinterpret_cast<uintptr_t>(this), reinterpret_cast<uintptr_t>(parent));
                return;
            }
            current = current->m_parent;
        }
    }

    if (m_parent)
    {
        // Detach from current parent
        auto& siblings = m_parent->m_children;
        std::erase(siblings, this);
    }

    m_parent = parent;

    if (parent)
    {
        parent->m_children.push_back(this);
    }
    
    SetTransformDirty();
}

void DeltaEngine::SceneComponent::UpdateTransformHierarchy(DirectX::XMMATRIX worldTransform)
{
    m_worldTransform = m_localTransform * worldTransform;
    OnTransformChanged();

    for (SceneComponent* child : m_children) {
        child->UpdateTransformHierarchy(m_worldTransform);
    }
}

void DeltaEngine::SceneComponent::UpdateTransform()
{
    if (m_parent) {
        XMMATRIX parentTransform = m_parent->m_worldTransform;
        UpdateTransformHierarchy(parentTransform);
    }
    else
    {
        UpdateTransformHierarchy(XMMatrixIdentity());
    }
}

void DeltaEngine::SceneComponent::SetTransformDirty()
{
    UpdateTransform();
}

void DeltaEngine::SceneComponent::SyncEulerFromMatrix()
{
    XMVECTOR scale, rotation, translation;
    if (XMMatrixDecompose(&scale, &rotation, &translation, m_localTransform))
    {
        auto euler = SimpleMath::Quaternion(rotation).ToEuler();
        m_eulerRotationCache = SimpleMath::Vector3(
            XMConvertToDegrees(euler.x),
            XMConvertToDegrees(euler.y),
            XMConvertToDegrees(euler.z));
    }
}

void DeltaEngine::SceneComponent::PostEditChangeProperty(const DProperty* prop)
{
    static const DProperty* s_localTransformProp =
        GetClass()->FindPropertyByName("m_localTransform");

    if (prop == s_localTransformProp)
    {
        SyncEulerFromMatrix();
        SetTransformDirty();
    }
}

void SceneComponent::PostRestore()
{
    SyncEulerFromMatrix();
    UpdateTransform();
}

void SceneComponent::OnAfterDeserialize()
{
    SyncEulerFromMatrix();
}
