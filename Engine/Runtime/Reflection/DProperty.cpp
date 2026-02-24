#include "Runtime/Reflection/DProperty.h"

#include "SimpleMath.h"

using namespace DeltaEngine;

// ---------------------------------------------------------------------------
// DProperty base
// ---------------------------------------------------------------------------

DProperty::DProperty(std::string name,
                     std::string type,
                     uint32_t offset,
                     uint32_t size)
    : m_name(std::move(name)),
      m_type(std::move(type)),
      m_offset(offset),
      m_size(size),
      m_next(nullptr),
      m_declaringClass(nullptr)
{
}

// ---------------------------------------------------------------------------
// DFloatProperty
// ---------------------------------------------------------------------------

DFloatProperty::DFloatProperty(std::string name, uint32_t offset)
    : DNumericProperty<float>(std::move(name), "float", offset)
{
}

EPropertyType DFloatProperty::GetPropertyType() const
{
    return EPropertyType::Float;
}

// ---------------------------------------------------------------------------
// DIntProperty
// ---------------------------------------------------------------------------

DIntProperty::DIntProperty(std::string name, uint32_t offset)
    : DNumericProperty<int32_t>(std::move(name), "int32_t", offset)
{
}

EPropertyType DIntProperty::GetPropertyType() const
{
    return EPropertyType::Int;
}

// ---------------------------------------------------------------------------
// DBoolProperty
// ---------------------------------------------------------------------------

DBoolProperty::DBoolProperty(std::string name, uint32_t offset)
    : DNumericProperty<bool>(std::move(name), "bool", offset)
{
}

EPropertyType DBoolProperty::GetPropertyType() const
{
    return EPropertyType::Bool;
}

// ---------------------------------------------------------------------------
// DDoubleProperty
// ---------------------------------------------------------------------------

DDoubleProperty::DDoubleProperty(std::string name, uint32_t offset)
    : DNumericProperty<double>(std::move(name), "double", offset)
{
}

EPropertyType DDoubleProperty::GetPropertyType() const
{
    return EPropertyType::Double;
}

// ---------------------------------------------------------------------------
// DStringProperty
// ---------------------------------------------------------------------------

DStringProperty::DStringProperty(std::string name, uint32_t offset)
    : DProperty(std::move(name), "std::string", offset, sizeof(std::string))
{
}

void DStringProperty::InitializeValue(void* address) const
{
    new (address) std::string();
}

void DStringProperty::DestroyValue(void* address) const
{
    static_cast<std::string*>(address)->~basic_string();
}

void DStringProperty::SetValue(void* instance, const void* field_value) const
{
    void* addr = static_cast<uint8_t*>(instance) + m_offset;
    *static_cast<std::string*>(addr) = field_value
        ? *static_cast<const std::string*>(field_value)
        : std::string{};
}

void* DStringProperty::GetValue(const void* instance) const
{
    return static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
}

void DStringProperty::CopyValue(void* dest, const void* src) const
{
    new (dest) std::string(*static_cast<const std::string*>(src));
}

bool DStringProperty::Identical(const void* a, const void* b) const
{
    return *static_cast<const std::string*>(a) == *static_cast<const std::string*>(b);
}

std::string DStringProperty::ToString(const void* address) const
{
    return *static_cast<const std::string*>(address);
}

EPropertyType DStringProperty::GetPropertyType() const
{
    return EPropertyType::String;
}

// ---------------------------------------------------------------------------
// DVector3Property
// ---------------------------------------------------------------------------

using Vector3 = DirectX::SimpleMath::Vector3;

DVector3Property::DVector3Property(std::string name, uint32_t offset)
    : DProperty(std::move(name), "Vector3", offset, sizeof(Vector3))
{
}

void DVector3Property::InitializeValue(void* address) const
{
    new (address) Vector3();
}

void DVector3Property::DestroyValue(void* address) const
{
}

void DVector3Property::SetValue(void* instance, const void* field_value) const
{
    void* addr = static_cast<uint8_t*>(instance) + m_offset;
    *static_cast<Vector3*>(addr) = field_value
        ? *static_cast<const Vector3*>(field_value)
        : Vector3{};
}

void* DVector3Property::GetValue(const void* instance) const
{
    return static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
}

void DVector3Property::CopyValue(void* dest, const void* src) const
{
    new (dest) Vector3(*static_cast<const Vector3*>(src));
}

bool DVector3Property::Identical(const void* a, const void* b) const
{
    const auto& va = *static_cast<const Vector3*>(a);
    const auto& vb = *static_cast<const Vector3*>(b);
    return va.x == vb.x && va.y == vb.y && va.z == vb.z;
}

std::string DVector3Property::ToString(const void* address) const
{
    const auto& v = *static_cast<const Vector3*>(address);
    return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
}

EPropertyType DVector3Property::GetPropertyType() const
{
    return EPropertyType::Vector3;
}

// ---------------------------------------------------------------------------
// DQuaternionProperty
// ---------------------------------------------------------------------------

using Quaternion = DirectX::SimpleMath::Quaternion;

DQuaternionProperty::DQuaternionProperty(std::string name, uint32_t offset)
    : DProperty(std::move(name), "Quaternion", offset, sizeof(Quaternion))
{
}

void DQuaternionProperty::InitializeValue(void* address) const
{
    new (address) Quaternion();
}

void DQuaternionProperty::DestroyValue(void* address) const
{
}

void DQuaternionProperty::SetValue(void* instance, const void* field_value) const
{
    void* addr = static_cast<uint8_t*>(instance) + m_offset;
    *static_cast<Quaternion*>(addr) = field_value
        ? *static_cast<const Quaternion*>(field_value)
        : Quaternion{};
}

void* DQuaternionProperty::GetValue(const void* instance) const
{
    return static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
}

void DQuaternionProperty::CopyValue(void* dest, const void* src) const
{
    new (dest) Quaternion(*static_cast<const Quaternion*>(src));
}

bool DQuaternionProperty::Identical(const void* a, const void* b) const
{
    const auto& qa = *static_cast<const Quaternion*>(a);
    const auto& qb = *static_cast<const Quaternion*>(b);
    return qa.x == qb.x && qa.y == qb.y && qa.z == qb.z && qa.w == qb.w;
}

std::string DQuaternionProperty::ToString(const void* address) const
{
    const auto& q = *static_cast<const Quaternion*>(address);
    return "(" + std::to_string(q.x) + ", " + std::to_string(q.y) + ", "
         + std::to_string(q.z) + ", " + std::to_string(q.w) + ")";
}

EPropertyType DQuaternionProperty::GetPropertyType() const
{
    return EPropertyType::Quaternion;
}
