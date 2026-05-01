#pragma once

#include "EngineIncludes.h"

#include <string>
#include <unordered_map>

DELTA_ENGINE_NS_BEGIN

class DProperty;
class ReflectionRegistry;
class AssetArchive;
class DObject;

class DStruct
{
    friend class ReflectionRegistry;

public:
    /// Describes a reflected struct or class layout.
    DStruct(std::string name,
            std::string superName,
            size_t structSize,
            size_t minAlignment);

    virtual ~DStruct() = default;

    /// Adds a property declared directly on this type.
    DProperty* AddProperty(DProperty* property);
    /// Finds a property by name on this type or one of its reflected bases.
    DELTAENGINE_API DProperty* FindPropertyByName(const std::string& name) const;

    /// Links this type to its reflected base type.
    void SetSuper(DStruct* super);
    /// Rebuilds the hierarchy property chain after all supers have been linked.
    void RebuildHierarchyChain();

    DELTAENGINE_API const std::string& GetName() const;
    DELTAENGINE_API const std::string& GetSuperName() const;
    DELTAENGINE_API DStruct* GetSuper() const;
    DELTAENGINE_API size_t GetStructSize() const;
    DELTAENGINE_API size_t GetMinAlignment() const;
    DELTAENGINE_API DProperty* GetProperties() const;
    DELTAENGINE_API DProperty* GetOwnProperties() const;

    /// Serializes the fields declared on this type and its reflected bases.
    DELTAENGINE_API void SerializeFields(AssetArchive& ar, void* basePtr);
    /// Serializes a reflected object instance through this type schema.
    DELTAENGINE_API void Serialize(AssetArchive& ar, DObject& obj);

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
    std::string m_superName;
    DStruct* m_super = nullptr;
    DProperty* m_properties = nullptr;
    DProperty* m_ownProperties = nullptr;
    size_t m_structSize;
    size_t m_minAlignment;
    std::unordered_map<std::string, std::string> m_metadata;
};

DELTA_ENGINE_NS_END
