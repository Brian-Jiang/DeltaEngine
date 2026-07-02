#include "Editor/Commands/EnumPropertyWire.h"

#include "Runtime/Reflection/DEnumProperty.h"
#include "Runtime/Reflection/DProperty.h"

#include <cstdint>
#include <string_view>

using namespace DeltaEngine;

namespace
{

bool IsUnsignedUnderlyingType(std::string_view underlyingType)
{
    return underlyingType.starts_with("uint") || underlyingType.starts_with("unsigned");
}

int64_t ReadSizedValue(const void* addr, uint32_t size, bool unsignedType)
{
    switch (size)
    {
    case 1:
        return unsignedType ? static_cast<int64_t>(*static_cast<const uint8_t*>(addr))
                            : static_cast<int64_t>(*static_cast<const int8_t*>(addr));
    case 2:
        return unsignedType ? static_cast<int64_t>(*static_cast<const uint16_t*>(addr))
                            : static_cast<int64_t>(*static_cast<const int16_t*>(addr));
    case 4:
        return unsignedType ? static_cast<int64_t>(*static_cast<const uint32_t*>(addr))
                            : static_cast<int64_t>(*static_cast<const int32_t*>(addr));
    case 8:
        return unsignedType ? static_cast<int64_t>(*static_cast<const uint64_t*>(addr))
                            : static_cast<int64_t>(*static_cast<const int64_t*>(addr));
    default:
        return 0;
    }
}

void WriteSizedValue(void* addr, uint32_t size, bool unsignedType, int64_t value)
{
    switch (size)
    {
    case 1:
        if (unsignedType)
            *static_cast<uint8_t*>(addr) = static_cast<uint8_t>(value);
        else
            *static_cast<int8_t*>(addr) = static_cast<int8_t>(value);
        break;
    case 2:
        if (unsignedType)
            *static_cast<uint16_t*>(addr) = static_cast<uint16_t>(value);
        else
            *static_cast<int16_t*>(addr) = static_cast<int16_t>(value);
        break;
    case 4:
        if (unsignedType)
            *static_cast<uint32_t*>(addr) = static_cast<uint32_t>(value);
        else
            *static_cast<int32_t*>(addr) = static_cast<int32_t>(value);
        break;
    case 8:
        if (unsignedType)
            *static_cast<uint64_t*>(addr) = static_cast<uint64_t>(value);
        else
            *static_cast<int64_t*>(addr) = value;
        break;
    default:
        break;
    }
}

bool ResolveEnumUnsignedType(const DProperty* prop)
{
    const auto* enumProp = static_cast<const DEnumPropertyBase*>(prop);
    if (DEnum* schema = enumProp->GetEnumSchema())
        return IsUnsignedUnderlyingType(schema->GetUnderlyingType());
    return false;
}

}

int64_t DeltaEngine::ReadEnumUnderlyingAsInt64(const DProperty* prop, const void* addr)
{
    if (!prop || !addr)
        return 0;

    return ReadSizedValue(addr, prop->GetSize(), ResolveEnumUnsignedType(prop));
}

void DeltaEngine::WriteEnumUnderlyingFromInt64(const DProperty* prop, void* addr, int64_t value)
{
    if (!prop || !addr)
        return;

    WriteSizedValue(addr, prop->GetSize(), ResolveEnumUnsignedType(prop), value);
}
