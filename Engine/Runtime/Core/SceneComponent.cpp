#include "Core/SceneComponent.h"

using namespace DirectX;
using namespace DeltaEngine;

SceneComponent::SceneComponent()
    : DComponent()
    , m_localTransform(XMMatrixIdentity())
    , m_worldTransform(XMMatrixIdentity())
{

}

SceneComponent::SceneComponent(std::string name)
    : DComponent(name)
    , m_localTransform(XMMatrixIdentity())
    , m_worldTransform(XMMatrixIdentity())
{
}

SceneComponent::SceneComponent(std::string name, std::shared_ptr<GameObject> gameObject)
    : DComponent(name, gameObject)
    , m_localTransform(XMMatrixIdentity())
    , m_worldTransform(XMMatrixIdentity())
{
}

SimpleMath::Vector3 DeltaEngine::SceneComponent::GetLocalPosition() const {
    return SimpleMath::Vector3(m_localTransform.r[3]);
}

DirectX::SimpleMath::Vector3 DeltaEngine::SceneComponent::GetWorldPosition() const {
    return SimpleMath::Vector3(m_worldTransform.r[3]);
}

void DeltaEngine::SceneComponent::SetLocalPosition(DirectX::SimpleMath::Vector3 position) {
    m_localTransform.r[3] = XMVectorSet(position.x, position.y, position.z, 1.0f);
    SetTransformDirty();
}

void DeltaEngine::SceneComponent::SetLocalPosition(DirectX::XMVECTOR position) {
    m_localTransform.r[3] = position;
    SetTransformDirty();
}

void DeltaEngine::SceneComponent::SetLocalPosition(float x, float y, float z) {
    m_localTransform.r[3] = XMVectorSet(x, y, z, 1.0f);
    SetTransformDirty();
}

void DeltaEngine::SceneComponent::SetWorldPosition(DirectX::SimpleMath::Vector3 position) {
    SetWorldPosition(position.x, position.y, position.z);
}

void DeltaEngine::SceneComponent::SetWorldPosition(DirectX::XMVECTOR position) {
    SetWorldPosition(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position));
}

void DeltaEngine::SceneComponent::SetWorldPosition(float x, float y, float z) {
    if (auto parent = m_parent.lock()) {
        auto position = XMVectorSet(x, y, z, 1.0f);
        XMVECTOR parentPosition = parent->m_worldTransform.r[3];
        auto localPositionParentSpace = position - parentPosition;
        SetLocalPosition(localPositionParentSpace);
    }
    else {
        SetLocalPosition(x, y, z);
    }
}

DirectX::SimpleMath::Quaternion DeltaEngine::SceneComponent::GetLocalRotation() const {
    XMVECTOR translation;
    XMVECTOR currentRotation;
    XMVECTOR scale;
    bool success = XMMatrixDecompose(&scale, &currentRotation, &translation, m_localTransform);
    if (!success) {

        return SimpleMath::Quaternion::Identity;
    }

    return SimpleMath::Quaternion(currentRotation);
}

DirectX::SimpleMath::Quaternion DeltaEngine::SceneComponent::GetWorldRotation() const {
    XMVECTOR translation;
    XMVECTOR currentRotation;
    XMVECTOR scale;
    bool success = XMMatrixDecompose(&scale, &currentRotation, &translation, m_worldTransform);
    if (!success) {

        return SimpleMath::Quaternion::Identity;
    }

    return SimpleMath::Quaternion(currentRotation);
}

void DeltaEngine::SceneComponent::SetLocalRotation(DirectX::SimpleMath::Quaternion rotation) {
    SetLocalRotation((XMVECTOR) rotation);
}

void DeltaEngine::SceneComponent::SetLocalRotation(DirectX::XMVECTOR rotation) {
    XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotation);
    XMVECTOR translation;
    XMVECTOR currentRotation;
    XMVECTOR scale;
    bool success = XMMatrixDecompose(&scale, &currentRotation, &translation, m_localTransform);
    if (!success) {

        return;
    }

    m_localTransform = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rotation) * XMMatrixTranslationFromVector(translation);
    SetTransformDirty();
}

void DeltaEngine::SceneComponent::SetWorldRotation(DirectX::SimpleMath::Quaternion rotation) {
    SetWorldRotation((XMVECTOR) rotation);
}

void DeltaEngine::SceneComponent::SetWorldRotation(DirectX::XMVECTOR rotation) {
    if (auto parent = m_parent.lock()) {
        auto parentWorldTransform = parent->m_worldTransform;
        XMVECTOR translation;
        XMVECTOR parentRotation;
        XMVECTOR scale;
        bool success = XMMatrixDecompose(&scale, &parentRotation, &translation, parentWorldTransform);
        if (!success) {

            return;
        }

        XMVECTOR parentInverseRotation = XMQuaternionInverse(parentRotation);
        auto localRotationParentSpace = XMQuaternionMultiply(rotation, parentInverseRotation);
        SetLocalRotation(localRotationParentSpace);
    }
    else {
        SetLocalRotation(rotation);
    }
}

SimpleMath::Vector3 DeltaEngine::SceneComponent::GetLocalScale() const {
    XMVECTOR translation;
    XMVECTOR rotation;
    XMVECTOR scale;
    bool success = XMMatrixDecompose(&scale, &rotation, &translation, m_localTransform);
    if (!success) {

        return SimpleMath::Vector3::One;
    }

    return SimpleMath::Vector3(scale);
}

DirectX::SimpleMath::Vector3 DeltaEngine::SceneComponent::GetWorldScale() const {
    XMVECTOR translation;
    XMVECTOR rotation;
    XMVECTOR scale;
    bool success = XMMatrixDecompose(&scale, &rotation, &translation, m_worldTransform);
    if (!success) {

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
    bool success = XMMatrixDecompose(&currentScale, &rotation, &translation, m_worldTransform);
    if (!success) {

        return;
    }

    m_localTransform = XMMatrixScalingFromVector(scale) * XMMatrixRotationQuaternion(rotation) * XMMatrixTranslationFromVector(translation);
    SetTransformDirty();
}

DirectX::SimpleMath::Vector3 DeltaEngine::SceneComponent::GetRight() const {
    return m_worldTransform.r[0];
}

DirectX::SimpleMath::Vector3 DeltaEngine::SceneComponent::GetUp() const {
    return m_worldTransform.r[1];
}

DirectX::SimpleMath::Vector3 DeltaEngine::SceneComponent::GetForward() const {
    return m_worldTransform.r[2];
}

void DeltaEngine::SceneComponent::SetParent(std::shared_ptr<SceneComponent> parent) {
    m_parent = parent;
    parent->m_children.push_back(shared_from_this());
    SetTransformDirty();
}

void DeltaEngine::SceneComponent::UpdateTransformHierarchy(DirectX::XMMATRIX worldTransform)
{
    m_worldTransform = m_localTransform * worldTransform;
    OnTransformChanged();

    for (std::shared_ptr<SceneComponent> child : m_children) {
        child->UpdateTransformHierarchy(m_worldTransform);
    }
}

void DeltaEngine::SceneComponent::UpdateTransform()
{
    if (auto parent = m_parent.lock()) {
        XMMATRIX parentTransform = parent->m_worldTransform;
        UpdateTransformHierarchy(parentTransform);
    }
    else {
        UpdateTransformHierarchy(XMMatrixIdentity());
    }
}

void DeltaEngine::SceneComponent::SetTransformDirty()
{
    UpdateTransform();
}
