#pragma once

#include "EngineIncludes.h"

#include <string>
#include <memory>
#include <type_traits>

DELTA_ENGINE_NS_BEGIN

class DClass;
class DObject;

enum class EPropertyType
{
    Float,
    Int,
    Bool,
    Double,
    String,
    Vector3,
    Quaternion,
    ObjectPtr,
};

class DProperty
{
    friend class DClass;

public:
    DProperty(std::string name,
        std::string type,
        uint32_t offset,
        uint32_t size
    );

    virtual ~DProperty() = default;

    virtual void InitializeValue(void* address) const = 0;
    virtual void DestroyValue(void* address) const = 0;
    virtual void SetValue(void* instance, const void* field_value) const = 0;
    virtual void* GetValue(const void* instance) const = 0;
    virtual void CopyValue(void* dest, const void* src) const = 0;
    virtual bool Identical(const void* a, const void* b) const = 0;
    virtual std::string ToString(const void* address) const = 0;
    virtual EPropertyType GetPropertyType() const = 0;

    const std::string& GetName() const { return m_name; }
    const std::string& GetType() const { return m_type; }
    uint32_t GetOffset() const { return m_offset; }
    uint32_t GetSize() const { return m_size; }
    DClass* GetDeclaringClass() const { return m_declaringClass; }
    DProperty* GetNext() const { return m_next; }

protected:
    std::string m_name;
    std::string m_type;
    uint32_t m_offset;
    uint32_t m_size;
    DClass* m_declaringClass;

    DProperty* m_next;
};


template <typename T>
concept NumericType = std::is_arithmetic_v<T>;


template <typename T>
    requires NumericType<T>
class DNumericProperty : public DProperty
{
public:
    DNumericProperty(std::string name,
                     std::string type,
                     uint32_t offset)
        : DProperty(std::move(name), std::move(type), offset, sizeof(T))
    {

    }

    void SetValue(void* instance, const void* field_value) const override
    {
        void* addr = static_cast<uint8_t*>(instance) + m_offset;
        *static_cast<T*>(addr) = field_value ? *static_cast<const T*>(field_value) : T {};
    }

    void* GetValue(const void* instance) const override
    {
        return static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
    }

    void InitializeValue(void* address) const override
    {
        new (address) T();
    }

    void DestroyValue(void* address) const override
    {

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
        return std::to_string(*static_cast<const T*>(address));
    }
};


class DFloatProperty : public DNumericProperty<float>
{
public:
    DFloatProperty(std::string name, uint32_t offset);
    EPropertyType GetPropertyType() const override;
};


class DIntProperty : public DNumericProperty<int32_t>
{
public:
    DIntProperty(std::string name, uint32_t offset);
    EPropertyType GetPropertyType() const override;
};


class DBoolProperty : public DNumericProperty<bool>
{
public:
    DBoolProperty(std::string name, uint32_t offset);
    EPropertyType GetPropertyType() const override;
};


class DDoubleProperty : public DNumericProperty<double>
{
public:
    DDoubleProperty(std::string name, uint32_t offset);
    EPropertyType GetPropertyType() const override;
};


class DStringProperty : public DProperty
{
public:
    DStringProperty(std::string name, uint32_t offset);

    void InitializeValue(void* address) const override;
    void DestroyValue(void* address) const override;
    void SetValue(void* instance, const void* field_value) const override;
    void* GetValue(const void* instance) const override;
    void CopyValue(void* dest, const void* src) const override;
    bool Identical(const void* a, const void* b) const override;
    std::string ToString(const void* address) const override;
    EPropertyType GetPropertyType() const override;
};


class DVector3Property : public DProperty
{
public:
    DVector3Property(std::string name, uint32_t offset);

    void InitializeValue(void* address) const override;
    void DestroyValue(void* address) const override;
    void SetValue(void* instance, const void* field_value) const override;
    void* GetValue(const void* instance) const override;
    void CopyValue(void* dest, const void* src) const override;
    bool Identical(const void* a, const void* b) const override;
    std::string ToString(const void* address) const override;
    EPropertyType GetPropertyType() const override;
};


class DQuaternionProperty : public DProperty
{
public:
    DQuaternionProperty(std::string name, uint32_t offset);

    void InitializeValue(void* address) const override;
    void DestroyValue(void* address) const override;
    void SetValue(void* instance, const void* field_value) const override;
    void* GetValue(const void* instance) const override;
    void CopyValue(void* dest, const void* src) const override;
    bool Identical(const void* a, const void* b) const override;
    std::string ToString(const void* address) const override;
    EPropertyType GetPropertyType() const override;
};


// Only for raw pointers to DObject-derived types
template <typename T>
// requires std::is_base_of_v<DObject, T>
class DObjectPtrProperty : public DProperty
{
public:
    DObjectPtrProperty(std::string name, std::string type, uint32_t offset)
        : DProperty(std::move(name), std::move(type), offset, sizeof(T*))
    {
    }

    void InitializeValue(void* address) const override
    {
        new (address) T*(nullptr);
    }

    void DestroyValue(void* address) const override
    {
        // No-op for raw pointers
    }

    void SetValue(void* instance, const void* field_value) const override
    {
        void* addr = static_cast<uint8_t*>(instance) + m_offset;
        *static_cast<T**>(addr) = field_value ? *static_cast<T* const*>(field_value) : nullptr;
    }

    void* GetValue(const void* instance) const override
    {
        return static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
    }

    void CopyValue(void* dest, const void* src) const override
    {
        new (dest) T*(*static_cast<T* const*>(src));
    }

    bool Identical(const void* a, const void* b) const override
    {
        return *static_cast<T* const*>(a) == *static_cast<T* const*>(b);
    }

    std::string ToString(const void* address) const override
    {
        T* ptr = *static_cast<T* const*>(address);
        if (ptr) {
            return "ObjectPtr(" + m_type + ")";
        }

        return "nullptr";
    }

    EPropertyType GetPropertyType() const override
    {
        return EPropertyType::ObjectPtr;
    }
};

DELTA_ENGINE_NS_END
