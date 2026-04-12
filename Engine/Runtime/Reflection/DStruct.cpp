#include "Runtime/Reflection/DStruct.h"

#include "Runtime/Reflection/DProperty.h"
#include "Serialization/AssetArchive.h"
#include "Core/DObject.h"

using namespace DeltaEngine;

DStruct::DStruct(std::string name,
                 std::string superName,
                 size_t structSize,
                 size_t minAlignment)
    : m_name(std::move(name))
    , m_superName(std::move(superName))
    , m_structSize(structSize)
    , m_minAlignment(minAlignment)
{
}

void DStruct::AddProperty(DProperty* property)
{
    property->m_declaringStruct = this;

    property->m_next = m_ownProperties;
    property->m_hierarchyNext = m_ownProperties;
    m_ownProperties = property;
}

DProperty* DStruct::FindPropertyByName(const std::string& name) const
{
    for (DProperty* prop = m_ownProperties; prop; prop = prop->m_next)
    {
        if (prop->m_name == name)
            return prop;
    }

    if (m_super)
        return m_super->FindPropertyByName(name);

    return nullptr;
}

void DStruct::SetSuper(DStruct* super)
{
    m_super = super;
}

void DStruct::RebuildHierarchyChain()
{
    if (!m_ownProperties)
        return;

    DProperty* tail = m_ownProperties;
    while (tail->m_next)
        tail = tail->m_next;
    tail->m_hierarchyNext = m_super ? m_super->GetProperties() : nullptr;
}

const std::string& DStruct::GetName() const { return m_name; }
const std::string& DStruct::GetSuperName() const { return m_superName; }
DStruct* DStruct::GetSuper() const { return m_super; }
size_t DStruct::GetStructSize() const { return m_structSize; }
size_t DStruct::GetMinAlignment() const { return m_minAlignment; }
DProperty* DStruct::GetProperties() const
{
    if (m_ownProperties)
        return m_ownProperties;
    return m_super ? m_super->GetProperties() : nullptr;
}
DProperty* DStruct::GetOwnProperties() const { return m_ownProperties; }

void DStruct::SerializeFields(AssetArchive& ar, void* basePtr)
{
    if (DStruct* parent = GetSuper())
        parent->SerializeFields(ar, basePtr);

    for (DProperty* prop = m_ownProperties; prop; prop = prop->GetNext())
        prop->Serialize(ar, basePtr);
}

void DStruct::Serialize(AssetArchive& ar, DObject& obj)
{
    SerializeFields(ar, &obj);
}
