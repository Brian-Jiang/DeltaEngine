#pragma once

#include "EngineIncludes.h"

#include <string>
#include <memory>
#include <type_traits>
#include <unordered_map>

#include "Core/DObject.h"
#include "Serialization/ScriptPointer.h"

DELTA_ENGINE_NS_BEGIN

class DStruct;
class DClass;
class AssetArchive;

enum class EPropertyType
{
    Float,
    Int,
    Bool,
    Double,
    String,
    WString,
    Vector3,
    Quaternion,
    Float4,
    Float4x4,
    ObjectPtr,
    SharedObjectPtr,
    BulkData,
};

class DProperty
{
    friend class DStruct;

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

    /// Returns the pointed-to DObject* for ObjectPtr/SharedObjectPtr, or nullptr for other types.
    virtual DObject* GetObjectPointer(const void* instance) const { return nullptr; }

    virtual void Serialize(AssetArchive& ar, void* objectPtr) = 0;

    const std::string& GetName() const { return m_name; }
    const std::string& GetType() const { return m_type; }
    uint32_t GetOffset() const { return m_offset; }
    uint32_t GetSize() const { return m_size; }
    DStruct* GetDeclaringStruct() const { return m_declaringStruct; }
    DProperty* GetNext() const { return m_next; }

    void SetMetadata(std::unordered_map<std::string, std::string> metadata)
    {
        m_metadata = std::move(metadata);
    }

    bool HasMeta(const std::string& key) const
    {
        return m_metadata.count(key) > 0;
    }

    DELTAENGINE_API std::string GetMeta(const std::string& key, const std::string& defaultVal = "") const;

protected:
    std::string m_name;
    std::string m_type;
    uint32_t m_offset;
    uint32_t m_size;
    DStruct* m_declaringStruct;

    DProperty* m_next;
    std::unordered_map<std::string, std::string> m_metadata;
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
        static_cast<DObject*>(instance)->MarkDirty();
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
    void Serialize(AssetArchive& ar, void* objectPtr) override;
};


class DIntProperty : public DNumericProperty<int32_t>
{
public:
    DIntProperty(std::string name, uint32_t offset);
    EPropertyType GetPropertyType() const override;
    void Serialize(AssetArchive& ar, void* objectPtr) override;
};


class DBoolProperty : public DNumericProperty<bool>
{
public:
    DBoolProperty(std::string name, uint32_t offset);
    EPropertyType GetPropertyType() const override;
    void Serialize(AssetArchive& ar, void* objectPtr) override;
};


class DDoubleProperty : public DNumericProperty<double>
{
public:
    DDoubleProperty(std::string name, uint32_t offset);
    EPropertyType GetPropertyType() const override;
    void Serialize(AssetArchive& ar, void* objectPtr) override;
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
    void Serialize(AssetArchive& ar, void* objectPtr) override;
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
    void Serialize(AssetArchive& ar, void* objectPtr) override;
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
    void Serialize(AssetArchive& ar, void* objectPtr) override;
};


class DObjectPtrPropertyBase : public DProperty
{
public:
    DObjectPtrPropertyBase(std::string name, std::string type, uint32_t offset, uint32_t size)
        : DProperty(std::move(name), std::move(type), offset, size) {}

    void Serialize(AssetArchive& ar, void* objectPtr) override;

    virtual void ResolvePointer(void* objectPtr, DObject* resolved) = 0;

    DELTAENGINE_API void SetUnresolvedPointer(void* objectPtr, const ScriptPointer& sp);
    DELTAENGINE_API ScriptPointer GetUnresolvedPointer(void* objectPtr) const;

protected:
    virtual DObject* GetRawPointer(const void* objectPtr) const = 0;
    std::unordered_map<void*, ScriptPointer> m_unresolvedPointers;
};


// Only for raw pointers to DObject-derived types
template <typename T>
// requires std::is_base_of_v<DObject, T>
class DObjectPtrProperty : public DObjectPtrPropertyBase
{
public:
    DObjectPtrProperty(std::string name, std::string type, uint32_t offset)
        : DObjectPtrPropertyBase(std::move(name), std::move(type), offset, sizeof(T*))
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

    DObject* GetObjectPointer(const void* instance) const override
    {
        T* ptr = *static_cast<T* const*>(GetValue(instance));
        if constexpr (DObjectDerived<T>)
            return static_cast<DObject*>(ptr);
        return nullptr;
    }

    DObject* GetRawPointer(const void* objectPtr) const override
    {
        T* ptr = *static_cast<T* const*>(GetValue(objectPtr));
        if constexpr (DObjectDerived<T>)
            return static_cast<DObject*>(ptr);
        return nullptr;
    }

    void ResolvePointer(void* objectPtr, DObject* resolved) override
    {
        void* addr = static_cast<uint8_t*>(objectPtr) + m_offset;
        *static_cast<T**>(addr) = static_cast<T*>(resolved);
        m_unresolvedPointers.erase(objectPtr);
    }
};


template <typename T>
class DSharedObjectPtrProperty : public DObjectPtrPropertyBase
{
public:
    DSharedObjectPtrProperty(std::string name, std::string type, uint32_t offset)
        : DObjectPtrPropertyBase(std::move(name), std::move(type), offset, sizeof(std::shared_ptr<T>))
    {
    }

    void InitializeValue(void* address) const override
    {
        new (address) std::shared_ptr<T>();
    }

    void DestroyValue(void* address) const override
    {
        static_cast<std::shared_ptr<T>*>(address)->~shared_ptr();
    }

    void SetValue(void* instance, const void* field_value) const override
    {
        void* addr = static_cast<uint8_t*>(instance) + m_offset;
        if (field_value)
            *static_cast<std::shared_ptr<T>*>(addr) = *static_cast<const std::shared_ptr<T>*>(field_value);
        else
            static_cast<std::shared_ptr<T>*>(addr)->reset();
    }

    void* GetValue(const void* instance) const override
    {
        return static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
    }

    void CopyValue(void* dest, const void* src) const override
    {
        new (dest) std::shared_ptr<T>(*static_cast<const std::shared_ptr<T>*>(src));
    }

    bool Identical(const void* a, const void* b) const override
    {
        return *static_cast<const std::shared_ptr<T>*>(a) == *static_cast<const std::shared_ptr<T>*>(b);
    }

    std::string ToString(const void* address) const override
    {
        const auto& ptr = *static_cast<const std::shared_ptr<T>*>(address);
        if (ptr)
            return "SharedPtr(" + m_type + ")";
        return "nullptr";
    }

    EPropertyType GetPropertyType() const override
    {
        return EPropertyType::SharedObjectPtr;
    }

    DObject* GetObjectPointer(const void* instance) const override
    {
        const auto& sp = *static_cast<const std::shared_ptr<T>*>(GetValue(instance));
        if constexpr (DObjectDerived<T>)
            return static_cast<DObject*>(sp.get());
        return nullptr;
    }

    DObject* GetRawPointer(const void* objectPtr) const override
    {
        const auto& sp = *static_cast<const std::shared_ptr<T>*>(GetValue(objectPtr));
        if constexpr (DObjectDerived<T>)
            return static_cast<DObject*>(sp.get());
        return nullptr;
    }

    void ResolvePointer(void* objectPtr, DObject* /*resolved*/) override
    {
        m_unresolvedPointers.erase(objectPtr);
    }
};


class DWStringProperty : public DProperty
{
public:
    DWStringProperty(std::string name, uint32_t offset);

    void InitializeValue(void* address) const override;
    void DestroyValue(void* address) const override;
    void SetValue(void* instance, const void* field_value) const override;
    void* GetValue(const void* instance) const override;
    void CopyValue(void* dest, const void* src) const override;
    bool Identical(const void* a, const void* b) const override;
    std::string ToString(const void* address) const override;
    EPropertyType GetPropertyType() const override;
    void Serialize(AssetArchive& ar, void* objectPtr) override;
};


class DFloat4Property : public DProperty
{
public:
    DFloat4Property(std::string name, uint32_t offset);

    void InitializeValue(void* address) const override;
    void DestroyValue(void* address) const override;
    void SetValue(void* instance, const void* field_value) const override;
    void* GetValue(const void* instance) const override;
    void CopyValue(void* dest, const void* src) const override;
    bool Identical(const void* a, const void* b) const override;
    std::string ToString(const void* address) const override;
    EPropertyType GetPropertyType() const override;
    void Serialize(AssetArchive& ar, void* objectPtr) override;
};


class DFloat4x4Property : public DProperty
{
public:
    DFloat4x4Property(std::string name, uint32_t offset);

    void InitializeValue(void* address) const override;
    void DestroyValue(void* address) const override;
    void SetValue(void* instance, const void* field_value) const override;
    void* GetValue(const void* instance) const override;
    void CopyValue(void* dest, const void* src) const override;
    bool Identical(const void* a, const void* b) const override;
    std::string ToString(const void* address) const override;
    EPropertyType GetPropertyType() const override;
    void Serialize(AssetArchive& ar, void* objectPtr) override;
};

DELTA_ENGINE_NS_END
