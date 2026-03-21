#include "Runtime/Reflection/DProperty.h"

#include "Runtime/Reflection/DStruct.h"
#include "Reflection/ReflectionRegistry.h"
#include "Serialization/AssetArchive.h"
#include "Assets/DPrimaryAsset.h"
#include "Runtime/Utils/StringUtils.h"

#include "SimpleMath.h"
#include <DirectXMath.h>
#include <cstring>
#include <vector>

using namespace DeltaEngine;

// ---------------------------------------------------------------------------
// DProperty base
// ---------------------------------------------------------------------------

DProperty::DProperty(std::string name,
                     std::string type,
                     uint32_t offset,
                     uint32_t size)
    : m_name(std::move(name)),
      m_type(std::move(type)),
      m_offset(offset),
      m_size(size),
      m_next(nullptr),
      m_declaringStruct(nullptr)
{
}

std::string DProperty::GetMeta(const std::string& key, const std::string& defaultVal) const
{
    auto it = m_metadata.find(key);
    return (it != m_metadata.end()) ? it->second : defaultVal;
}

// ---------------------------------------------------------------------------
// DFloatProperty
// ---------------------------------------------------------------------------

DFloatProperty::DFloatProperty(std::string name, uint32_t offset)
    : DNumericProperty<float>(std::move(name), "float", offset)
{
}

EPropertyType DFloatProperty::GetPropertyType() const
{
    return EPropertyType::Float;
}

void DFloatProperty::Serialize(AssetArchive& ar, void* objectPtr)
{
    ar.Serialize(GetName(), *static_cast<float*>(GetValue(objectPtr)));
}

void DFloatProperty::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    ar.SerializeElement(*static_cast<float*>(elementAddr));
}

// ---------------------------------------------------------------------------
// DIntProperty
// ---------------------------------------------------------------------------

DIntProperty::DIntProperty(std::string name, uint32_t offset)
    : DNumericProperty<int32_t>(std::move(name), "int32_t", offset)
{
}

EPropertyType DIntProperty::GetPropertyType() const
{
    return EPropertyType::Int;
}

void DIntProperty::Serialize(AssetArchive& ar, void* objectPtr)
{
    ar.Serialize(GetName(), *static_cast<int*>(GetValue(objectPtr)));
}

void DIntProperty::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    ar.SerializeElement(*static_cast<int*>(elementAddr));
}

// ---------------------------------------------------------------------------
// DBoolProperty
// ---------------------------------------------------------------------------

DBoolProperty::DBoolProperty(std::string name, uint32_t offset)
    : DNumericProperty<bool>(std::move(name), "bool", offset)
{
}

EPropertyType DBoolProperty::GetPropertyType() const
{
    return EPropertyType::Bool;
}

void DBoolProperty::Serialize(AssetArchive& ar, void* objectPtr)
{
    ar.Serialize(GetName(), *static_cast<bool*>(GetValue(objectPtr)));
}

void DBoolProperty::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    ar.SerializeElement(*static_cast<bool*>(elementAddr));
}

// ---------------------------------------------------------------------------
// DDoubleProperty
// ---------------------------------------------------------------------------

DDoubleProperty::DDoubleProperty(std::string name, uint32_t offset)
    : DNumericProperty<double>(std::move(name), "double", offset)
{
}

EPropertyType DDoubleProperty::GetPropertyType() const
{
    return EPropertyType::Double;
}

void DDoubleProperty::Serialize(AssetArchive& ar, void* objectPtr)
{
    ar.Serialize(GetName(), *static_cast<double*>(GetValue(objectPtr)));
}

void DDoubleProperty::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    ar.SerializeElement(*static_cast<double*>(elementAddr));
}

// ---------------------------------------------------------------------------
// DStringProperty
// ---------------------------------------------------------------------------

DStringProperty::DStringProperty(std::string name, uint32_t offset)
    : DProperty(std::move(name), "std::string", offset, sizeof(std::string))
{
}

void DStringProperty::InitializeValue(void* address) const
{
    new (address) std::string();
}

void DStringProperty::DestroyValue(void* address) const
{
    static_cast<std::string*>(address)->~basic_string();
}

void DStringProperty::SetValue(void* instance, const void* field_value) const
{
    void* addr = static_cast<uint8_t*>(instance) + m_offset;
    *static_cast<std::string*>(addr) = field_value
        ? *static_cast<const std::string*>(field_value)
        : std::string{};
    static_cast<DObject*>(instance)->MarkDirty();
}

void* DStringProperty::GetValue(const void* instance) const
{
    return static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
}

void DStringProperty::CopyValue(void* dest, const void* src) const
{
    new (dest) std::string(*static_cast<const std::string*>(src));
}

bool DStringProperty::Identical(const void* a, const void* b) const
{
    return *static_cast<const std::string*>(a) == *static_cast<const std::string*>(b);
}

std::string DStringProperty::ToString(const void* address) const
{
    return *static_cast<const std::string*>(address);
}

EPropertyType DStringProperty::GetPropertyType() const
{
    return EPropertyType::String;
}

void DStringProperty::Serialize(AssetArchive& ar, void* objectPtr)
{
    ar.Serialize(GetName(), *static_cast<std::string*>(GetValue(objectPtr)));
}

void DStringProperty::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    ar.SerializeElement(*static_cast<std::string*>(elementAddr));
}

// ---------------------------------------------------------------------------
// DVector3Property
// ---------------------------------------------------------------------------

using Vector3 = DirectX::SimpleMath::Vector3;

DVector3Property::DVector3Property(std::string name, uint32_t offset)
    : DProperty(std::move(name), "Vector3", offset, sizeof(Vector3))
{
}

void DVector3Property::InitializeValue(void* address) const
{
    new (address) Vector3();
}

void DVector3Property::DestroyValue(void* address) const
{
}

void DVector3Property::SetValue(void* instance, const void* field_value) const
{
    void* addr = static_cast<uint8_t*>(instance) + m_offset;
    *static_cast<Vector3*>(addr) = field_value
        ? *static_cast<const Vector3*>(field_value)
        : Vector3{};
    static_cast<DObject*>(instance)->MarkDirty();
}

void* DVector3Property::GetValue(const void* instance) const
{
    return static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
}

void DVector3Property::CopyValue(void* dest, const void* src) const
{
    new (dest) Vector3(*static_cast<const Vector3*>(src));
}

bool DVector3Property::Identical(const void* a, const void* b) const
{
    const auto& va = *static_cast<const Vector3*>(a);
    const auto& vb = *static_cast<const Vector3*>(b);
    return va.x == vb.x && va.y == vb.y && va.z == vb.z;
}

std::string DVector3Property::ToString(const void* address) const
{
    const auto& v = *static_cast<const Vector3*>(address);
    return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
}

EPropertyType DVector3Property::GetPropertyType() const
{
    return EPropertyType::Vector3;
}

void DVector3Property::Serialize(AssetArchive& ar, void* objectPtr)
{
    ar.Serialize(GetName(), *static_cast<Vector3*>(GetValue(objectPtr)));
}

void DVector3Property::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    ar.SerializeElement(*static_cast<Vector3*>(elementAddr));
}

// ---------------------------------------------------------------------------
// DQuaternionProperty
// ---------------------------------------------------------------------------

using Quaternion = DirectX::SimpleMath::Quaternion;

DQuaternionProperty::DQuaternionProperty(std::string name, uint32_t offset)
    : DProperty(std::move(name), "Quaternion", offset, sizeof(Quaternion))
{
}

void DQuaternionProperty::InitializeValue(void* address) const
{
    new (address) Quaternion();
}

void DQuaternionProperty::DestroyValue(void* address) const
{
}

void DQuaternionProperty::SetValue(void* instance, const void* field_value) const
{
    void* addr = static_cast<uint8_t*>(instance) + m_offset;
    *static_cast<Quaternion*>(addr) = field_value
        ? *static_cast<const Quaternion*>(field_value)
        : Quaternion{};
    static_cast<DObject*>(instance)->MarkDirty();
}

void* DQuaternionProperty::GetValue(const void* instance) const
{
    return static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
}

void DQuaternionProperty::CopyValue(void* dest, const void* src) const
{
    new (dest) Quaternion(*static_cast<const Quaternion*>(src));
}

bool DQuaternionProperty::Identical(const void* a, const void* b) const
{
    const auto& qa = *static_cast<const Quaternion*>(a);
    const auto& qb = *static_cast<const Quaternion*>(b);
    return qa.x == qb.x && qa.y == qb.y && qa.z == qb.z && qa.w == qb.w;
}

std::string DQuaternionProperty::ToString(const void* address) const
{
    const auto& q = *static_cast<const Quaternion*>(address);
    return "(" + std::to_string(q.x) + ", " + std::to_string(q.y) + ", "
         + std::to_string(q.z) + ", " + std::to_string(q.w) + ")";
}

EPropertyType DQuaternionProperty::GetPropertyType() const
{
    return EPropertyType::Quaternion;
}

void DQuaternionProperty::Serialize(AssetArchive& ar, void* objectPtr)
{
    ar.Serialize(GetName(), *static_cast<Quaternion*>(GetValue(objectPtr)));
}

void DQuaternionProperty::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    ar.SerializeElement(*static_cast<Quaternion*>(elementAddr));
}

// ---------------------------------------------------------------------------
// DWStringProperty
// ---------------------------------------------------------------------------

DWStringProperty::DWStringProperty(std::string name, uint32_t offset)
    : DProperty(std::move(name), "std::wstring", offset, sizeof(std::wstring))
{
}

void DWStringProperty::InitializeValue(void* address) const
{
    new (address) std::wstring();
}

void DWStringProperty::DestroyValue(void* address) const
{
    static_cast<std::wstring*>(address)->~basic_string();
}

void DWStringProperty::SetValue(void* instance, const void* field_value) const
{
    void* addr = static_cast<uint8_t*>(instance) + m_offset;
    *static_cast<std::wstring*>(addr) = field_value
        ? *static_cast<const std::wstring*>(field_value)
        : std::wstring{};
    static_cast<DObject*>(instance)->MarkDirty();
}

void* DWStringProperty::GetValue(const void* instance) const
{
    return static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
}

void DWStringProperty::CopyValue(void* dest, const void* src) const
{
    new (dest) std::wstring(*static_cast<const std::wstring*>(src));
}

bool DWStringProperty::Identical(const void* a, const void* b) const
{
    return *static_cast<const std::wstring*>(a) == *static_cast<const std::wstring*>(b);
}

std::string DWStringProperty::ToString(const void* address) const
{
    const auto& ws = *static_cast<const std::wstring*>(address);
    return StringUtils::WStringToUtf8(ws);
}

EPropertyType DWStringProperty::GetPropertyType() const
{
    return EPropertyType::WString;
}

void DWStringProperty::Serialize(AssetArchive& ar, void* objectPtr)
{
    ar.Serialize(GetName(), *static_cast<std::wstring*>(GetValue(objectPtr)));
}

void DWStringProperty::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    ar.SerializeElement(*static_cast<std::wstring*>(elementAddr));
}

// ---------------------------------------------------------------------------
// DFloat4Property  (stores as XMFLOAT4 in memory)
// ---------------------------------------------------------------------------

using namespace DirectX;

DFloat4Property::DFloat4Property(std::string name, uint32_t offset)
    : DProperty(std::move(name), "XMFLOAT4", offset, sizeof(XMFLOAT4))
{
}

void DFloat4Property::InitializeValue(void* address) const
{
    new (address) XMFLOAT4(0.f, 0.f, 0.f, 0.f);
}

void DFloat4Property::DestroyValue(void* address) const
{
}

void DFloat4Property::SetValue(void* instance, const void* field_value) const
{
    void* addr = static_cast<uint8_t*>(instance) + m_offset;
    if (field_value)
        XMStoreFloat4(static_cast<XMFLOAT4*>(addr), *static_cast<const XMVECTOR*>(field_value));
    else
        *static_cast<XMFLOAT4*>(addr) = XMFLOAT4(0.f, 0.f, 0.f, 0.f);
    static_cast<DObject*>(instance)->MarkDirty();
}

void* DFloat4Property::GetValue(const void* instance) const
{
    return static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
}

void DFloat4Property::CopyValue(void* dest, const void* src) const
{
    *static_cast<XMFLOAT4*>(dest) = *static_cast<const XMFLOAT4*>(src);
}

bool DFloat4Property::Identical(const void* a, const void* b) const
{
    const auto& fa = *static_cast<const XMFLOAT4*>(a);
    const auto& fb = *static_cast<const XMFLOAT4*>(b);
    return fa.x == fb.x && fa.y == fb.y && fa.z == fb.z && fa.w == fb.w;
}

std::string DFloat4Property::ToString(const void* address) const
{
    const auto& f = *static_cast<const XMFLOAT4*>(address);
    return "(" + std::to_string(f.x) + ", " + std::to_string(f.y) + ", "
         + std::to_string(f.z) + ", " + std::to_string(f.w) + ")";
}

EPropertyType DFloat4Property::GetPropertyType() const
{
    return EPropertyType::Float4;
}

void DFloat4Property::Serialize(AssetArchive& ar, void* objectPtr)
{
    ar.Serialize(GetName(), *static_cast<XMFLOAT4*>(GetValue(objectPtr)));
}

void DFloat4Property::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    ar.SerializeElement(*static_cast<XMFLOAT4*>(elementAddr));
}

// ---------------------------------------------------------------------------
// DFloat4x4Property  (stores as XMFLOAT4X4 in memory)
// ---------------------------------------------------------------------------

DFloat4x4Property::DFloat4x4Property(std::string name, uint32_t offset)
    : DProperty(std::move(name), "XMFLOAT4X4", offset, sizeof(XMFLOAT4X4))
{
}

void DFloat4x4Property::InitializeValue(void* address) const
{
    XMStoreFloat4x4(static_cast<XMFLOAT4X4*>(address), XMMatrixIdentity());
}

void DFloat4x4Property::DestroyValue(void* address) const
{
}

void DFloat4x4Property::SetValue(void* instance, const void* field_value) const
{
    void* addr = static_cast<uint8_t*>(instance) + m_offset;
    if (field_value)
        XMStoreFloat4x4(static_cast<XMFLOAT4X4*>(addr), *static_cast<const XMMATRIX*>(field_value));
    else
        XMStoreFloat4x4(static_cast<XMFLOAT4X4*>(addr), XMMatrixIdentity());
    static_cast<DObject*>(instance)->MarkDirty();
}

void* DFloat4x4Property::GetValue(const void* instance) const
{
    return static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
}

void DFloat4x4Property::CopyValue(void* dest, const void* src) const
{
    *static_cast<XMFLOAT4X4*>(dest) = *static_cast<const XMFLOAT4X4*>(src);
}

bool DFloat4x4Property::Identical(const void* a, const void* b) const
{
    return memcmp(a, b, sizeof(XMFLOAT4X4)) == 0;
}

std::string DFloat4x4Property::ToString(const void* address) const
{
    return "XMFLOAT4X4(...)";
}

EPropertyType DFloat4x4Property::GetPropertyType() const
{
    return EPropertyType::Float4x4;
}

void DFloat4x4Property::Serialize(AssetArchive& ar, void* objectPtr)
{
    ar.Serialize(GetName(), *static_cast<XMFLOAT4X4*>(GetValue(objectPtr)));
}

void DFloat4x4Property::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    ar.SerializeElement(*static_cast<XMFLOAT4X4*>(elementAddr));
}

// ---------------------------------------------------------------------------
// DStructProperty
// ---------------------------------------------------------------------------

namespace
{
void CollectStructProps(DStruct* ds, void* base, std::vector<std::pair<DProperty*, void*>>& out)
{
    if (DStruct* p = ds->GetSuper())
        CollectStructProps(p, base, out);
    for (DProperty* prop = ds->GetOwnProperties(); prop; prop = prop->GetNext())
        out.emplace_back(prop, static_cast<uint8_t*>(base) + prop->GetOffset());
}
} // namespace

DStructProperty::DStructProperty(std::string name, uint32_t offset, uint32_t fieldSize, std::string structTypeName)
    : DProperty(std::move(name), structTypeName, offset, fieldSize)
    , m_structTypeName(std::move(structTypeName))
    , m_fieldSize(fieldSize)
{
}

DStruct* DStructProperty::GetSchema() const
{
    if (!m_cachedSchema)
        m_cachedSchema = GetReflectionRegistry().FindStructByName(m_structTypeName);
    return m_cachedSchema;
}

void DStructProperty::InitializeValue(void* address) const
{
    DStruct* ds = GetSchema();
    if (!ds)
        return;
    std::vector<std::pair<DProperty*, void*>> pairs;
    CollectStructProps(ds, address, pairs);
    for (auto& [prop, addr] : pairs)
        prop->InitializeValue(addr);
}

void DStructProperty::DestroyValue(void* address) const
{
    DStruct* ds = GetSchema();
    if (!ds)
        return;
    std::vector<std::pair<DProperty*, void*>> pairs;
    CollectStructProps(ds, address, pairs);
    for (auto it = pairs.rbegin(); it != pairs.rend(); ++it)
        it->first->DestroyValue(it->second);
}

void DStructProperty::SetValue(void* instance, const void* field_value) const
{
    void* addr = static_cast<uint8_t*>(instance) + m_offset;
    if (field_value)
        CopyValue(addr, field_value);
    else
        InitializeValue(addr);
    static_cast<DObject*>(instance)->MarkDirty();
}

void* DStructProperty::GetValue(const void* instance) const
{
    return static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
}

void DStructProperty::CopyValue(void* dest, const void* src) const
{
    DStruct* ds = GetSchema();
    if (!ds)
        return;
    std::vector<std::pair<DProperty*, void*>> pairs;
    CollectStructProps(ds, dest, pairs);
    std::vector<std::pair<DProperty*, void*>> srcPairs;
    CollectStructProps(ds, const_cast<void*>(src), srcPairs);
    for (size_t i = 0; i < pairs.size() && i < srcPairs.size(); ++i)
        pairs[i].first->CopyValue(pairs[i].second, srcPairs[i].second);
}

bool DStructProperty::Identical(const void* a, const void* b) const
{
    return std::memcmp(a, b, m_fieldSize) == 0;
}

std::string DStructProperty::ToString(const void* /*address*/) const
{
    return "{ " + m_structTypeName + " }";
}

EPropertyType DStructProperty::GetPropertyType() const
{
    return EPropertyType::Struct;
}

void DStructProperty::Serialize(AssetArchive& ar, void* objectPtr)
{
    DStruct* s = GetSchema();
    if (!s)
        return;
    void* fieldPtr = static_cast<uint8_t*>(objectPtr) + m_offset;
    if (ar.IsSaving())
    {
        ar.BeginNestedObject(GetName());
        s->SerializeFields(ar, fieldPtr);
        ar.EndNestedObject();
    }
    else
    {
        if (ar.BeginNestedObjectLoad(GetName()))
        {
            s->SerializeFields(ar, fieldPtr);
            ar.EndNestedObject();
        }
    }
}

void DStructProperty::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    DStruct* s = GetSchema();
    if (!s)
        return;
    if (ar.IsSaving())
    {
        ar.BeginObject(s->GetName());
        s->SerializeFields(ar, elementAddr);
        ar.EndObject();
    }
    else
    {
        (void)ar.BeginObjectLoad();
        s->SerializeFields(ar, elementAddr);
        ar.EndObject();
    }
}

// ---------------------------------------------------------------------------
// DObjectPtrPropertyBase
// ---------------------------------------------------------------------------

void DObjectPtrPropertyBase::Serialize(AssetArchive& ar, void* objectPtr)
{
    if (ar.IsSaving()) {
        DObject* target = GetRawPointer(objectPtr);
        ScriptPointer sp;
        if (target) {
            sp.m_objectId = target->GetObjectId();
            if (DPrimaryAsset* owningAsset = target->GetOwningAsset())
                sp.m_assetId = owningAsset->GetAssetId();
        }
        ar.Serialize(GetName(), sp);
    } else {
        ScriptPointer sp;
        ar.Serialize(GetName(), sp);
        m_unresolvedPointers[objectPtr] = sp;
    }
}

void DObjectPtrPropertyBase::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    if (ar.IsSaving()) {
        DObject* target = GetRawPointer(elementAddr);
        ScriptPointer sp;
        if (target) {
            sp.m_objectId = target->GetObjectId();
            if (DPrimaryAsset* owningAsset = target->GetOwningAsset())
                sp.m_assetId = owningAsset->GetAssetId();
        }
        ar.SerializeElement(sp);
    } else {
        ScriptPointer sp;
        ar.SerializeElement(sp);
        m_unresolvedPointers[elementAddr] = sp;
    }
}

void DObjectPtrPropertyBase::SetUnresolvedPointer(void* objectPtr, const ScriptPointer& sp)
{
    m_unresolvedPointers[objectPtr] = sp;
}

ScriptPointer DObjectPtrPropertyBase::GetUnresolvedPointer(void* objectPtr) const
{
    auto it = m_unresolvedPointers.find(objectPtr);
    return (it != m_unresolvedPointers.end()) ? it->second : ScriptPointer{};
}
