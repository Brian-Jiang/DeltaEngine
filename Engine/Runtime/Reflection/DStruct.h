#pragma once

#include "EngineIncludes.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

class DProperty;
class ReflectionRegistry;
class AssetArchive;
class DObject;

class DStruct
{
    friend class ReflectionRegistry;

public:
    DStruct(std::string name,
            std::string superName,
            size_t structSize,
            size_t minAlignment);

    virtual ~DStruct() = default;

    void AddProperty(DProperty* property);
    DELTAENGINE_API DProperty* FindPropertyByName(const std::string& name) const;

    void SetSuper(DStruct* super);

    DELTAENGINE_API const std::string& GetName() const;
    DELTAENGINE_API const std::string& GetSuperName() const;
    DELTAENGINE_API DStruct* GetSuper() const;
    DELTAENGINE_API size_t GetStructSize() const;
    DELTAENGINE_API size_t GetMinAlignment() const;
    DELTAENGINE_API DProperty* GetProperties() const;
    DELTAENGINE_API DProperty* GetOwnProperties() const;

    DELTAENGINE_API void Serialize(AssetArchive& ar, DObject& obj);

protected:
    std::string m_name;
    std::string m_superName;
    DStruct* m_super = nullptr;
    DProperty* m_properties = nullptr;
    DProperty* m_ownProperties = nullptr;
    size_t m_structSize;
    size_t m_minAlignment;
};

DELTA_ENGINE_NS_END
