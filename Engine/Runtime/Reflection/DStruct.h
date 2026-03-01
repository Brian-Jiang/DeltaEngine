#pragma once

#include "EngineIncludes.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

class DProperty;
class ReflectionRegistry;

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
    DProperty* FindPropertyByName(const std::string& name) const;

    void SetSuper(DStruct* super);

    const std::string& GetName() const;
    const std::string& GetSuperName() const;
    DStruct* GetSuper() const;
    size_t GetStructSize() const;
    size_t GetMinAlignment() const;
    DProperty* GetProperties() const;
    DProperty* GetOwnProperties() const;

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
