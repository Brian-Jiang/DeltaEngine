#include "Runtime/Reflection/DEnum.h"
#include "Runtime/Reflection/DEnumProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

using namespace DeltaEngine;

DEnum::DEnum(std::string name, std::string underlyingType)
    : m_name(std::move(name)),
      m_underlyingType(std::move(underlyingType))
{
}

void DEnum::AddEntry(std::string name, int64_t value)
{
    m_entries.push_back(DEnumEntry{ std::move(name), value });
}

const DEnumEntry* DEnum::FindEntryByValue(int64_t value) const
{
    for (const DEnumEntry& entry : m_entries)
    {
        if (entry.value == value)
            return &entry;
    }

    return nullptr;
}

DEnumPropertyBase::DEnumPropertyBase(
    std::string name,
    std::string enumTypeName,
    uint32_t offset,
    uint32_t size)
    : DProperty(std::move(name), std::move(enumTypeName), offset, size)
{
}

DEnum* DEnumPropertyBase::GetEnumSchema() const
{
    return GetReflectionRegistry().FindEnumByName(GetType());
}
