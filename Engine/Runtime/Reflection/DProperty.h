#pragma once

#include "EngineIncludes.h"

#include <string>
#include <memory>

DELTA_ENGINE_NS_BEGIN

class DClass;

enum class EPropertyType
{
    Float,
    Int,
    Bool,
    String,
    // ...
};

class DProperty
{
    friend class DClass;

public:
    DProperty(std::string name,
        std::string type,
        uint32_t offset,
        uint32_t size,
        void (*setter)(void* instance, std::shared_ptr<void> field_value),
        void* (*getter)(void* instance)
    );

protected:
    virtual void InitializeValue(void* address) const = 0;
    virtual void DestroyValue(void* address) const = 0;
    virtual void CopyValue(void* dest, const void* src) const = 0;
    virtual bool Identical(const void* a, const void* b) const = 0;
    virtual std::string ToString(const void* address) const = 0;
    virtual EPropertyType GetPropertyType() const = 0;

private:
    std::string m_name;
    std::string m_type;
    uint32_t m_offset;
    uint32_t m_size;
    void (*m_setter)(void* instance, std::shared_ptr<void> field_value);
    void* (*m_getter)(void* instance);

    DProperty* m_next;
    DClass* m_declaringClass;
};

DELTA_ENGINE_NS_END
