#include "Runtime/Core/SceneComponent.h"

#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Logging/LogChannels.h"

#include <cstdint>
#include <vector>

using namespace DirectX;
using namespace DeltaEngine;

SceneComponent::SceneComponent()
    : DComponent()
    , m_localPosition(0.0f, 0.0f, 0.0f)
    , m_localRotation(SimpleMath::Quaternion::Identity)
    , m_localScale(1.0f, 1.0f, 1.0f)
    , m_localEulerAngles(0.0f, 0.0f, 0.0f)
    , m_parent(nullptr)
    , m_localMatrix(XMMatrixIdentity())
    , m_worldPosition(0.0f, 0.0f, 0.0f)
    , m_worldRotation(SimpleMath::Quaternion::Identity)
    , m_worldScale(1.0f, 1.0f, 1.0f)
{
}

SimpleMath::Vector3 SceneComponent::GetLocalPosition() const
{
    return m_localPosition;
}

DirectX::SimpleMath::Vector3 DeltaEngine::SceneComponent::GetWorldPosition() const
{
    EnsureWorldTRS();
    return m_worldPosition;
}

void SceneComponent::SetLocalPosition(DirectX::SimpleMath::Vector3 position)
{
    m_localPosition = position;
    SetTransformDirty();
}

void SceneComponent::SetLocalPosition(DirectX::XMVECTOR position)
{
    m_localPosition = SimpleMath::Vector3(XMVectorGetX(position), XMVectorGetY(position), XMVectorGetZ(position));
    SetTransformDirty();
}

void SceneComponent::SetLocalPosition(float x, float y, float z)
{
    m_localPosition = SimpleMath::Vector3(x, y, z);
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
        // Convert the world-space point into the parent's local space via the full
        // inverse of the parent's world matrix (accounts for parent rotation and scale).
        XMMATRIX invParent = XMMatrixInverse(nullptr, m_parent->GetWorldTransform());
        XMVECTOR localPosition = XMVector3TransformCoord(XMVectorSet(x, y, z, 1.0f), invParent);
        SetLocalPosition(localPosition);
    }
    else
    {
        SetLocalPosition(x, y, z);
    }
}

DirectX::SimpleMath::Quaternion DeltaEngine::SceneComponent::GetLocalRotation() const
{
    return m_localRotation;
}

DirectX::SimpleMath::Vector3 DeltaEngine::SceneComponent::GetLocalRotationEulerAngles() const
{
    return m_localEulerAngles;
}

DirectX::SimpleMath::Quaternion DeltaEngine::SceneComponent::GetWorldRotation() const
{
    EnsureWorldTRS();
    return m_worldRotation;
}

void SceneComponent::SetLocalRotation(DirectX::SimpleMath::Quaternion rotation)
{
    SetLocalRotation((XMVECTOR) rotation);
}

void SceneComponent::SetLocalRotation(DirectX::XMVECTOR rotation)
{
    m_localRotation = SimpleMath::Quaternion(rotation);
    SimpleMath::Vector3 eulerRadian = m_localRotation.ToEuler();
    m_localEulerAngles = SimpleMath::Vector3(
        XMConvertToDegrees(eulerRadian.x),
        XMConvertToDegrees(eulerRadian.y),
        XMConvertToDegrees(eulerRadian.z));
    SetTransformDirty();
}

void DeltaEngine::SceneComponent::SetLocalRotation(DirectX::SimpleMath::Vector3 eulerAngles)
{
    // The euler angles are the user-facing source here — preserve them exactly (e.g. 370 vs 10)
    // and derive the quaternion from them, rather than round-tripping through the quaternion.
    m_localEulerAngles = eulerAngles;
    XMVECTOR rotationQuat = XMQuaternionRotationRollPitchYaw(
        XMConvertToRadians(eulerAngles.x),
        XMConvertToRadians(eulerAngles.y),
        XMConvertToRadians(eulerAngles.z));
    m_localRotation = SimpleMath::Quaternion(rotationQuat);
    SetTransformDirty();
}

void DeltaEngine::SceneComponent::SetWorldRotation(DirectX::SimpleMath::Quaternion rotation)
{
    SetWorldRotation((XMVECTOR) rotation);
}

void DeltaEngine::SceneComponent::SetWorldRotation(DirectX::XMVECTOR rotation)
{
    if (m_parent)
    {
        SimpleMath::Quaternion parentRotation = m_parent->GetWorldRotation();
        XMVECTOR parentInverseRotation = XMQuaternionInverse((XMVECTOR) parentRotation);
        XMVECTOR localRotationParentSpace = XMQuaternionMultiply(rotation, parentInverseRotation);
        SetLocalRotation(localRotationParentSpace);
    }
    else
    {
        SetLocalRotation(rotation);
    }
}

SimpleMath::Vector3 DeltaEngine::SceneComponent::GetLocalScale() const
{
    return m_localScale;
}

DirectX::SimpleMath::Vector3 DeltaEngine::SceneComponent::GetWorldScale() const
{
    EnsureWorldTRS();
    return m_worldScale;
}

void DeltaEngine::SceneComponent::SetLocalScale(DirectX::SimpleMath::Vector3 scale)
{
    m_localScale = scale;
    SetTransformDirty();
}

void DeltaEngine::SceneComponent::SetLocalScale(float x, float y, float z)
{
    m_localScale = SimpleMath::Vector3(x, y, z);
    SetTransformDirty();
}

DirectX::SimpleMath::Vector3 DeltaEngine::SceneComponent::GetRight() const
{
    return GetWorldTransform().r[0];
}

DirectX::SimpleMath::Vector3 DeltaEngine::SceneComponent::GetUp() const
{
    return GetWorldTransform().r[1];
}

DirectX::SimpleMath::Vector3 DeltaEngine::SceneComponent::GetForward() const
{
    return GetWorldTransform().r[2];
}

XMMATRIX DeltaEngine::SceneComponent::GetWorldTransform() const
{
    // Not cached — composed fresh by walking up the parent chain iteratively.
    // world = L_self * L_parent * ... * L_root
    XMMATRIX world = XMMatrixIdentity();
    for (const SceneComponent* node = this; node; node = node->m_parent)
    {
        world = world * node->GetLocalMatrix();
    }
    return world;
}

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
        while (current)
        {
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

    // Local transform is unchanged, but this node's world position moves with the new parent.
    MarkWorldTRSDirtySubtree();
}

DirectX::XMMATRIX DeltaEngine::SceneComponent::GetLocalMatrix() const
{
    EnsureLocalClean();
    return m_localMatrix;
}

void DeltaEngine::SceneComponent::EnsureLocalClean() const
{
    if (!m_localDirty)
    {
        return;
    }

    m_localMatrix = XMMatrixScalingFromVector((XMVECTOR) m_localScale)
        * XMMatrixRotationQuaternion((XMVECTOR) m_localRotation)
        * XMMatrixTranslationFromVector((XMVECTOR) m_localPosition);
    m_localDirty = false;
}

void DeltaEngine::SceneComponent::EnsureWorldTRS() const
{
    if (!m_worldTRSDirty)
    {
        return;
    }

    XMMATRIX world = GetWorldTransform();

    XMVECTOR scale;
    XMVECTOR rotation;
    XMVECTOR translation;
    if (XMMatrixDecompose(&scale, &rotation, &translation, world))
    {
        m_worldPosition = SimpleMath::Vector3(translation);
        m_worldRotation = SimpleMath::Quaternion(rotation);
        m_worldScale = SimpleMath::Vector3(scale);
    }
    else
    {
        DLOG(LogCore, ELogLevel::Warning,
            "SceneComponent::EnsureWorldTRS: XMMatrixDecompose failed on world transform (this={}) — using translation row, identity rotation, unit scale",
            reinterpret_cast<uintptr_t>(this));
        m_worldPosition = SimpleMath::Vector3(world.r[3]);
        m_worldRotation = SimpleMath::Quaternion::Identity;
        m_worldScale = SimpleMath::Vector3::One;
    }

    m_worldTRSDirty = false;
}

void DeltaEngine::SceneComponent::MarkWorldTRSDirtySubtree()
{
    // Iterative subtree walk (explicit stack, no recursion). A local/parent change on this node
    // invalidates the world transform of the whole subtree; fire OnTransformChanged per node so
    // render proxies refresh (GetWorldTransform recomputes from the updated locals on demand).
    std::vector<SceneComponent*> stack{ this };
    while (!stack.empty())
    {
        SceneComponent* node = stack.back();
        stack.pop_back();

        node->m_worldTRSDirty = true;
        node->OnTransformChanged();

        for (SceneComponent* child : node->m_children)
        {
            stack.push_back(child);
        }
    }
}

void DeltaEngine::SceneComponent::SetTransformDirty()
{
    m_localDirty = true;
    MarkWorldTRSDirtySubtree();
}

void DeltaEngine::SceneComponent::PostEditChangeProperty(const DProperty* prop)
{
    static const DProperty* s_positionProp = GetClass()->FindPropertyByName("m_localPosition");
    static const DProperty* s_rotationProp = GetClass()->FindPropertyByName("m_localRotation");
    static const DProperty* s_scaleProp = GetClass()->FindPropertyByName("m_localScale");
    static const DProperty* s_eulerProp = GetClass()->FindPropertyByName("m_localEulerAngles");

    if (prop == s_eulerProp)
    {
        // Euler edited: rebuild the quaternion source of truth from it.
        XMVECTOR quat = XMQuaternionRotationRollPitchYaw(
            XMConvertToRadians(m_localEulerAngles.x),
            XMConvertToRadians(m_localEulerAngles.y),
            XMConvertToRadians(m_localEulerAngles.z));
        m_localRotation = SimpleMath::Quaternion(quat);
    }
    else if (prop == s_rotationProp)
    {
        // Quaternion edited: refresh the euler hint to stay coherent.
        SimpleMath::Vector3 eulerRadian = m_localRotation.ToEuler();
        m_localEulerAngles = SimpleMath::Vector3(
            XMConvertToDegrees(eulerRadian.x),
            XMConvertToDegrees(eulerRadian.y),
            XMConvertToDegrees(eulerRadian.z));
    }

    if (prop == s_positionProp || prop == s_rotationProp || prop == s_scaleProp || prop == s_eulerProp)
    {
        m_localDirty = true;
        MarkWorldTRSDirtySubtree();
    }
}

void SceneComponent::PostRestore()
{
    m_localDirty = true;
    MarkWorldTRSDirtySubtree();
}

void SceneComponent::OnAfterDeserialize()
{
    // Trust the deserialized euler hint (it may legitimately differ from the quaternion's
    // principal value); just invalidate the derived caches so they rebuild lazily.
    m_localDirty = true;
    m_worldTRSDirty = true;
}
