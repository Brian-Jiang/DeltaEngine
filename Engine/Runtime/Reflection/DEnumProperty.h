#pragma once

#include "Runtime/Reflection/DEnum.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Serialization/AssetArchive.h"

DELTA_ENGINE_NS_BEGIN

template <typename T>
class DEnumProperty : public DProperty
{
    static_assert(std::is_enum_v<T>, "DEnumProperty requires an enum type");

public:
    DEnumProperty(std::string name, uint32_t offset, std::string enumTypeName)
        : DProperty(std::move(name), enumTypeName, offset, sizeof(T)),
          m_enumTypeName(std::move(enumTypeName))
    {
    }

    void InitializeValue(void* address) const override
    {
        new (address) T();
    }

    void DestroyValue(void* address) const override
    {
    }

    void SetValue(void* instance, const void* field_value) const override
    {
        void* addr = static_cast<uint8_t*>(instance) + m_offset;
        *static_cast<T*>(addr) = field_value ? *static_cast<const T*>(field_value) : T {};
        static_cast<DObject*>(instance)->MarkDirty();
    }

    void* GetValue(const void* instance) const override
    {
        return static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
    }

    void CopyValue(void* dest, const void* src) const override
    {
        new (dest) T(*static_cast<const T*>(src));
    }

    bool Identical(const void* a, const void* b) const override
    {
        return *static_cast<const T*>(a) == *static_cast<const T*>(b);
    }

    std::string ToString(const void* address) const override
    {
        using Underlying = std::underlying_type_t<T>;
        const Underlying underlying = static_cast<Underlying>(*static_cast<const T*>(address));

        if (DEnum* schema = GetEnum())
        {
            if (const DEnumEntry* entry = schema->FindEntryByValue(static_cast<int64_t>(underlying)))
                return entry->name;
        }

        return std::to_string(static_cast<int64_t>(underlying));
    }

    EPropertyType GetPropertyType() const override
    {
        return EPropertyType::Enum;
    }

    void Serialize(AssetArchive& ar, void* objectPtr) override
    {
        DELTA_VERIFY(objectPtr != nullptr);
        using Underlying = std::underlying_type_t<T>;
        T& field = *static_cast<T*>(GetValue(objectPtr));
        int wire = static_cast<int>(static_cast<Underlying>(field));
        ar.Serialize(GetName(), wire);
        if (ar.IsLoading())
            field = static_cast<T>(static_cast<Underlying>(wire));
    }

    void SerializeElement(AssetArchive& ar, void* elementAddr) override
    {
        DELTA_VERIFY(elementAddr != nullptr);
        using Underlying = std::underlying_type_t<T>;
        T& field = *static_cast<T*>(elementAddr);
        int wire = static_cast<int>(static_cast<Underlying>(field));
        ar.SerializeElement(wire);
        if (ar.IsLoading())
            field = static_cast<T>(static_cast<Underlying>(wire));
    }

    DEnum* GetEnum() const
    {
        return GetReflectionRegistry().FindEnumByName(m_enumTypeName);
    }

private:
    std::string m_enumTypeName;
};

DELTA_ENGINE_NS_END
