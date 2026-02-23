#include "Runtime/Reflection/DProperty.h"

using namespace DeltaEngine;

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

DFloatProperty::DFloatProperty(std::string name, uint32_t offset)
    : DNumericProperty<float>(std::move(name), "float", offset)
{
}

EPropertyType DFloatProperty::GetPropertyType() const
{
    return EPropertyType::Float;
}

DStringProperty::DStringProperty(std::string name,
                                 std::string type,
                                 uint32_t offset,
                                 uint32_t size)
    : DProperty(std::move(name), std::move(type), offset, size)
{
}

void DStringProperty::InitializeValue(void* address) const
{
    new (address) std::string();
}

void DStringProperty::DestroyValue(void* address) const
{
    static_cast<std::string*>(address)->std::string::~string();
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

DObjectProperty::DObjectProperty(std::string name,
                                 std::string type,
                                 uint32_t offset,
                                 uint32_t size)
    : DProperty(std::move(name), std::move(type), offset, size)
{
}

void DObjectProperty::InitializeValue(void* address) const
{
    new (address) std::shared_ptr<void>();
}
