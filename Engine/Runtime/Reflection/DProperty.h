#pragma once

#include "EngineIncludes.h"

#include <string>
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
    BulkData,
    Vector,
    Struct,
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

    /// Returns the pointed-to DObject* for ObjectPtr, or nullptr for other types.
    virtual DObject* GetObjectPointer(const void* instance) const { return nullptr; }

    virtual void Serialize(AssetArchive& ar, void* objectPtr) = 0;

    /// Serialize the value at elementAddr directly (no GetValue / GetName).
    /// Used by DVectorProperty to serialize each element.
    virtual void SerializeElement(AssetArchive& ar, void* elementAddr) = 0;

    const std::string& GetName() const { return m_name; }
    const std::string& GetType() const { return m_type; }
    uint32_t GetOffset() const { return m_offset; }
    uint32_t GetSize() const { return m_size; }
    DStruct* GetDeclaringStruct() const { return m_declaringStruct; }
    DProperty* GetNext() const { return m_next; }
    DProperty* GetHierarchyNext() const { return m_hierarchyNext; }

    void SetMetadata(std::unordered_map<std::string, std::string> metadata)
    {
        m_metadata = std::move(metadata);
    }

    bool HasMeta(const std::string& key) const
    {
        return m_metadata.count(key) > 0;
    }

    DELTAENGINE_API std::string GetMeta(const std::string& key, const std::string& defaultVal = "") const;

    bool IsEditorOnly() const { return bEditorOnly; }

    bool IsHiddenInDetails() const { return bHideInDetails; }

    bool bEditorOnly = false;
    bool bHideInDetails = false;

protected:
    std::string m_name;
    std::string m_type;
    uint32_t m_offset;
    uint32_t m_size;
    DStruct* m_declaringStruct;

    DProperty* m_next;
    DProperty* m_hierarchyNext;
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
    void SerializeElement(AssetArchive& ar, void* elementAddr) override;
};


class DIntProperty : public DNumericProperty<int32_t>
{
public:
    DIntProperty(std::string name, uint32_t offset);
    EPropertyType GetPropertyType() const override;
    void Serialize(AssetArchive& ar, void* objectPtr) override;
    void SerializeElement(AssetArchive& ar, void* elementAddr) override;
};


class DBoolProperty : public DNumericProperty<bool>
{
public:
    DBoolProperty(std::string name, uint32_t offset);
    EPropertyType GetPropertyType() const override;
    void Serialize(AssetArchive& ar, void* objectPtr) override;
    void SerializeElement(AssetArchive& ar, void* elementAddr) override;
};


class DDoubleProperty : public DNumericProperty<double>
{
public:
    DDoubleProperty(std::string name, uint32_t offset);
    EPropertyType GetPropertyType() const override;
    void Serialize(AssetArchive& ar, void* objectPtr) override;
    void SerializeElement(AssetArchive& ar, void* elementAddr) override;
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
    void SerializeElement(AssetArchive& ar, void* elementAddr) override;
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
    void SerializeElement(AssetArchive& ar, void* elementAddr) override;
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
    void SerializeElement(AssetArchive& ar, void* elementAddr) override;
};


class DObjectPtrPropertyBase : public DProperty
{
public:
    DObjectPtrPropertyBase(std::string name, std::string type, uint32_t offset, uint32_t size)
        : DProperty(std::move(name), std::move(type), offset, size) {}

    void Serialize(AssetArchive& ar, void* objectPtr) override;
    void SerializeElement(AssetArchive& ar, void* elementAddr) override;

    virtual void ResolvePointer(void* objectPtr, DObject* resolved) = 0;

    DELTAENGINE_API void SetUnresolvedPointer(void* objectPtr, const ScriptPointer& sp);
    DELTAENGINE_API ScriptPointer GetUnresolvedPointer(void* objectPtr) const;

protected:
    virtual DObject* GetRawPointer(const void* objectPtr) const = 0;
    std::unordered_map<void*, ScriptPointer> m_unresolvedPointers;
};


// Only for raw pointers to DObject-derived types
template <typename T>
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
    void SerializeElement(AssetArchive& ar, void* elementAddr) override;
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
    void SerializeElement(AssetArchive& ar, void* elementAddr) override;
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
    void SerializeElement(AssetArchive& ar, void* elementAddr) override;
};


class DELTAENGINE_API DStructProperty : public DProperty
{
public:
    DStructProperty(std::string name, uint32_t offset, uint32_t fieldSize, std::string structTypeName);

    void InitializeValue(void* address) const override;
    void DestroyValue(void* address) const override;
    void SetValue(void* instance, const void* field_value) const override;
    void* GetValue(const void* instance) const override;
    void CopyValue(void* dest, const void* src) const override;
    bool Identical(const void* a, const void* b) const override;
    std::string ToString(const void* address) const override;
    EPropertyType GetPropertyType() const override;
    void Serialize(AssetArchive& ar, void* objectPtr) override;
    void SerializeElement(AssetArchive& ar, void* elementAddr) override;

    DStruct* GetSchema() const;

private:
    std::string m_structTypeName;
    uint32_t m_fieldSize = 0;
    mutable DStruct* m_cachedSchema = nullptr;
};

DELTA_ENGINE_NS_END
