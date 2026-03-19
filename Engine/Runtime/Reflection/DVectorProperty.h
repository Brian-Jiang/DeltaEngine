#pragma once

#include "EngineIncludes.h"

#include "Reflection/DProperty.h"
#include "Serialization/AssetArchive.h"

#include <cstring>
#include <memory>
#include <string>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class DVectorPropertyBase : public DProperty
{
public:
    DVectorPropertyBase(std::string name, uint32_t offset, uint32_t size)
        : DProperty(std::move(name), "std::vector", offset, size)
    {
    }

    virtual size_t GetSize(const void* instance) const = 0;
    virtual void* GetElementAddress(void* instance, size_t index) const = 0;
    virtual const DProperty* GetInnerProperty() const = 0;
};

template <typename T>
class DVectorProperty : public DVectorPropertyBase
{
public:
    DVectorProperty(std::string name, uint32_t offset,
                    std::unique_ptr<DProperty> innerProp)
        : DVectorPropertyBase(std::move(name), offset,
                              static_cast<uint32_t>(sizeof(std::vector<T>)))
        , m_innerProperty(std::move(innerProp))
    {
    }

    void InitializeValue(void* address) const override
    {
        new (address) std::vector<T>();
    }

    void DestroyValue(void* address) const override
    {
        static_cast<std::vector<T>*>(address)->~vector();
    }

    void SetValue(void* instance, const void* field_value) const override
    {
        void* addr = static_cast<uint8_t*>(instance) + m_offset;
        *static_cast<std::vector<T>*>(addr) = field_value
            ? *static_cast<const std::vector<T>*>(field_value)
            : std::vector<T>{};
        static_cast<DObject*>(instance)->MarkDirty();
    }

    void* GetValue(const void* instance) const override
    {
        return static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
    }

    void CopyValue(void* dest, const void* src) const override
    {
        new (dest) std::vector<T>(*static_cast<const std::vector<T>*>(src));
    }

    bool Identical(const void* a, const void* b) const override
    {
        const auto& va = *static_cast<const std::vector<T>*>(a);
        const auto& vb = *static_cast<const std::vector<T>*>(b);
        if (va.size() != vb.size())
            return false;
        for (size_t i = 0; i < va.size(); ++i)
        {
            if constexpr (requires(const T& x, const T& y) { x == y; })
            {
                if (!(va[i] == vb[i]))
                    return false;
            }
            else
            {
                if (std::memcmp(&va[i], &vb[i], sizeof(T)) != 0)
                    return false;
            }
        }
        return true;
    }

    std::string ToString(const void* address) const override
    {
        const auto& vec = *static_cast<const std::vector<T>*>(address);
        return "vector[" + std::to_string(vec.size()) + "]";
    }

    EPropertyType GetPropertyType() const override
    {
        return EPropertyType::Vector;
    }

    void Serialize(AssetArchive& ar, void* objectPtr) override
    {
        auto& vec = *static_cast<std::vector<T>*>(GetValue(objectPtr));
        ar.Serialize(GetName(), vec, *m_innerProperty);
    }

    void SerializeElement(AssetArchive& ar, void* elementAddr) override
    {
        auto& vec = *static_cast<std::vector<T>*>(elementAddr);
        ar.SerializeNested(vec, *m_innerProperty);
    }

    size_t GetSize(const void* instance) const override
    {
        return static_cast<const std::vector<T>*>(GetValue(instance))->size();
    }

    void* GetElementAddress(void* instance, size_t index) const override
    {
        auto* vec = static_cast<std::vector<T>*>(GetValue(instance));
        return (index < vec->size()) ? &(*vec)[index] : nullptr;
    }

    const DProperty* GetInnerProperty() const override { return m_innerProperty.get(); }

private:
    std::unique_ptr<DProperty> m_innerProperty;
};

DELTA_ENGINE_NS_END
