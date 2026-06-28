#include "Runtime/Reflection/DProperty.h"

#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/Delegates/DynamicDelegate.h"
#include "Runtime/Core/GC/DObjectRegistry.h"
#include "Runtime/Reflection/DStruct.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Serialization/AssetArchive.h"
#include "Runtime/Utils/StringUtils.h"

#include <DirectXCollision.h>
#include <DirectXMath.h>
#include <SimpleMath.h>

#include <cstring>
#include <filesystem>
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
      m_hierarchyNext(nullptr),
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
    DELTA_VERIFY(objectPtr != nullptr);
    ar.Serialize(GetName(), *static_cast<float*>(GetValue(objectPtr)));
}

void DFloatProperty::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    DELTA_VERIFY(elementAddr != nullptr);
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
    DELTA_VERIFY(objectPtr != nullptr);
    ar.Serialize(GetName(), *static_cast<int*>(GetValue(objectPtr)));
}

void DIntProperty::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    DELTA_VERIFY(elementAddr != nullptr);
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
    DELTA_VERIFY(objectPtr != nullptr);
    ar.Serialize(GetName(), *static_cast<bool*>(GetValue(objectPtr)));
}

void DBoolProperty::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    DELTA_VERIFY(elementAddr != nullptr);
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
    DELTA_VERIFY(objectPtr != nullptr);
    ar.Serialize(GetName(), *static_cast<double*>(GetValue(objectPtr)));
}

void DDoubleProperty::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    DELTA_VERIFY(elementAddr != nullptr);
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
    DELTA_VERIFY(objectPtr != nullptr);
    ar.Serialize(GetName(), *static_cast<std::string*>(GetValue(objectPtr)));
}

void DStringProperty::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    DELTA_VERIFY(elementAddr != nullptr);
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
    DELTA_VERIFY(objectPtr != nullptr);
    ar.Serialize(GetName(), *static_cast<Vector3*>(GetValue(objectPtr)));
}

void DVector3Property::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    DELTA_VERIFY(elementAddr != nullptr);
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
    DELTA_VERIFY(objectPtr != nullptr);
    ar.Serialize(GetName(), *static_cast<Quaternion*>(GetValue(objectPtr)));
}

void DQuaternionProperty::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    DELTA_VERIFY(elementAddr != nullptr);
    ar.SerializeElement(*static_cast<Quaternion*>(elementAddr));
}

// ---------------------------------------------------------------------------
// DBoundingBoxProperty
// ---------------------------------------------------------------------------

using BoundingBox = DirectX::BoundingBox;

DBoundingBoxProperty::DBoundingBoxProperty(std::string name, uint32_t offset)
    : DProperty(std::move(name), "BoundingBox", offset, sizeof(BoundingBox))
{
}

void DBoundingBoxProperty::InitializeValue(void* address) const
{
    new (address) BoundingBox();
}

void DBoundingBoxProperty::DestroyValue(void* address) const
{
}

void DBoundingBoxProperty::SetValue(void* instance, const void* field_value) const
{
    void* addr = static_cast<uint8_t*>(instance) + m_offset;
    *static_cast<BoundingBox*>(addr) = field_value
        ? *static_cast<const BoundingBox*>(field_value)
        : BoundingBox{};
    static_cast<DObject*>(instance)->MarkDirty();
}

void* DBoundingBoxProperty::GetValue(const void* instance) const
{
    return static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
}

void DBoundingBoxProperty::CopyValue(void* dest, const void* src) const
{
    new (dest) BoundingBox(*static_cast<const BoundingBox*>(src));
}

bool DBoundingBoxProperty::Identical(const void* a, const void* b) const
{
    const auto& ba = *static_cast<const BoundingBox*>(a);
    const auto& bb = *static_cast<const BoundingBox*>(b);
    return ba.Center.x  == bb.Center.x  && ba.Center.y  == bb.Center.y  && ba.Center.z  == bb.Center.z
        && ba.Extents.x == bb.Extents.x && ba.Extents.y == bb.Extents.y && ba.Extents.z == bb.Extents.z;
}

std::string DBoundingBoxProperty::ToString(const void* address) const
{
    const auto& b = *static_cast<const BoundingBox*>(address);
    return "Center=(" + std::to_string(b.Center.x)  + ", " + std::to_string(b.Center.y)  + ", " + std::to_string(b.Center.z)  + "), "
           "Extents=(" + std::to_string(b.Extents.x) + ", " + std::to_string(b.Extents.y) + ", " + std::to_string(b.Extents.z) + ")";
}

EPropertyType DBoundingBoxProperty::GetPropertyType() const
{
    return EPropertyType::BoundingBox;
}

void DBoundingBoxProperty::Serialize(AssetArchive& ar, void* objectPtr)
{
    DELTA_VERIFY(objectPtr != nullptr);
    ar.Serialize(GetName(), *static_cast<BoundingBox*>(GetValue(objectPtr)));
}

void DBoundingBoxProperty::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    DELTA_VERIFY(elementAddr != nullptr);
    ar.SerializeElement(*static_cast<BoundingBox*>(elementAddr));
}

// ---------------------------------------------------------------------------
// DFilesystemPathProperty
// ---------------------------------------------------------------------------

DFilesystemPathProperty::DFilesystemPathProperty(std::string name, uint32_t offset)
    : DProperty(std::move(name), "std::filesystem::path", offset, sizeof(std::filesystem::path))
{
}

void DFilesystemPathProperty::InitializeValue(void* address) const
{
    new (address) std::filesystem::path();
}

void DFilesystemPathProperty::DestroyValue(void* address) const
{
    static_cast<std::filesystem::path*>(address)->~path();
}

void DFilesystemPathProperty::SetValue(void* instance, const void* field_value) const
{
    void* addr = static_cast<uint8_t*>(instance) + m_offset;
    *static_cast<std::filesystem::path*>(addr) = field_value
        ? *static_cast<const std::filesystem::path*>(field_value)
        : std::filesystem::path{};
    static_cast<DObject*>(instance)->MarkDirty();
}

void* DFilesystemPathProperty::GetValue(const void* instance) const
{
    return static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
}

void DFilesystemPathProperty::CopyValue(void* dest, const void* src) const
{
    new (dest) std::filesystem::path(*static_cast<const std::filesystem::path*>(src));
}

bool DFilesystemPathProperty::Identical(const void* a, const void* b) const
{
    return *static_cast<const std::filesystem::path*>(a) == *static_cast<const std::filesystem::path*>(b);
}

std::string DFilesystemPathProperty::ToString(const void* address) const
{
    return StringUtils::PathToUtf8(*static_cast<const std::filesystem::path*>(address));
}

EPropertyType DFilesystemPathProperty::GetPropertyType() const
{
    return EPropertyType::FilesystemPath;
}

void DFilesystemPathProperty::Serialize(AssetArchive& ar, void* objectPtr)
{
    DELTA_VERIFY(objectPtr != nullptr);
    auto* p = static_cast<std::filesystem::path*>(GetValue(objectPtr));
    std::string utf8;
    if (ar.IsSaving())
        utf8 = StringUtils::PathToUtf8(*p);
    ar.Serialize(GetName(), utf8);
    if (ar.IsLoading())
        *p = StringUtils::Utf8ToPath(utf8);
}

void DFilesystemPathProperty::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    DELTA_VERIFY(elementAddr != nullptr);
    auto* p = static_cast<std::filesystem::path*>(elementAddr);
    std::string utf8;
    if (ar.IsSaving())
        utf8 = StringUtils::PathToUtf8(*p);
    ar.SerializeElement(utf8);
    if (ar.IsLoading())
        *p = StringUtils::Utf8ToPath(utf8);
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
    DELTA_VERIFY(objectPtr != nullptr);
    ar.Serialize(GetName(), *static_cast<XMFLOAT4*>(GetValue(objectPtr)));
}

void DFloat4Property::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    DELTA_VERIFY(elementAddr != nullptr);
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
    DELTA_VERIFY(objectPtr != nullptr);
    ar.Serialize(GetName(), *static_cast<XMFLOAT4X4*>(GetValue(objectPtr)));
}

void DFloat4x4Property::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    DELTA_VERIFY(elementAddr != nullptr);
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
    DELTA_VERIFY(address != nullptr);

    DStruct* ds = GetSchema();
    if (!ds)
    {
        DLOG(LogReflection, ELogLevel::Warning,
             "DStructProperty::InitializeValue skipped: unknown struct type '{}' for property '{}' (expected FindStructByName match)",
             m_structTypeName, GetName());
        return;
    }
    std::vector<std::pair<DProperty*, void*>> pairs;
    CollectStructProps(ds, address, pairs);
    for (auto& [prop, addr] : pairs)
        prop->InitializeValue(addr);
}

void DStructProperty::DestroyValue(void* address) const
{
    DELTA_VERIFY(address != nullptr);

    DStruct* ds = GetSchema();
    if (!ds)
    {
        DLOG(LogReflection, ELogLevel::Warning,
             "DStructProperty::DestroyValue skipped: unknown struct type '{}' for property '{}' (expected FindStructByName match)",
             m_structTypeName, GetName());
        return;
    }
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
    DELTA_VERIFY(dest != nullptr);
    DELTA_VERIFY(src != nullptr);

    DStruct* ds = GetSchema();
    if (!ds)
    {
        DLOG(LogReflection, ELogLevel::Warning,
             "DStructProperty::CopyValue skipped: unknown struct type '{}' for property '{}' (expected FindStructByName match)",
             m_structTypeName, GetName());
        return;
    }
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
    DELTA_VERIFY(objectPtr != nullptr);

    DStruct* s = GetSchema();
    if (!s)
    {
        DLOG(LogReflection, ELogLevel::Warning,
             "DStructProperty::Serialize skipped: unknown struct type '{}' for property '{}' (expected registered DSTRUCT)",
             m_structTypeName, GetName());
        return;
    }
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
    DELTA_VERIFY(elementAddr != nullptr);

    DStruct* s = GetSchema();
    if (!s)
    {
        DLOG(LogReflection, ELogLevel::Warning,
             "DStructProperty::SerializeElement skipped: unknown struct type '{}' for property '{}' (expected registered DSTRUCT)",
             m_structTypeName, GetName());
        return;
    }
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
    DELTA_VERIFY(objectPtr != nullptr);

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
        m_unresolvedPointers[GetValue(objectPtr)] = sp;
    }
}

void DObjectPtrPropertyBase::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    DELTA_VERIFY(elementAddr != nullptr);

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

namespace
{

ScriptPointer MakeScriptPointer(DObject* target)
{
    ScriptPointer sp;
    if (target)
    {
        sp.m_objectId = target->GetObjectId();
        if (DPrimaryAsset* owningAsset = target->GetOwningAsset())
            sp.m_assetId = owningAsset->GetAssetId();
    }
    return sp;
}

void SerializeDelegateBindingArray(
    AssetArchive& ar,
    const std::string& arrayKey,
    bool namedArray,
    FDynamicMulticastDelegate* delegate,
    std::unordered_map<void*, std::vector<FUnresolvedDelegateBinding>>* unresolvedMap,
    void* fieldAddr)
{
    if (ar.IsSaving())
    {
        const std::vector<FDynamicDelegateBinding>& bindings = delegate->GetBindings();
        if (namedArray)
            ar.BeginArray(arrayKey, bindings.size());
        else
            ar.BeginNestedArray(bindings.size());

        for (const FDynamicDelegateBinding& binding : bindings)
        {
            DObject* target = GetDObjectRegistry().Resolve(binding.m_objectHandle);
            ScriptPointer sp = MakeScriptPointer(target);
            std::string functionName = binding.m_functionName;

            ar.BeginObject("DynamicDelegateBinding");
            ar.Serialize("object", sp);
            ar.Serialize("functionName", functionName);
            ar.EndObject();
        }
        ar.EndArray();
    }
    else
    {
        const size_t count = namedArray ? ar.BeginArrayLoad(arrayKey) : ar.BeginNestedArrayLoad();
        std::vector<FUnresolvedDelegateBinding> loaded;
        loaded.reserve(count);

        for (size_t i = 0; i < count; ++i)
        {
            (void)ar.BeginObjectLoad();
            FUnresolvedDelegateBinding entry;
            ar.Serialize("object", entry.m_object);
            ar.Serialize("functionName", entry.m_functionName);
            ar.EndObject();
            loaded.push_back(std::move(entry));
        }
        ar.EndArray();

        if (unresolvedMap)
            (*unresolvedMap)[fieldAddr] = std::move(loaded);
    }
}

} // namespace

// ---------------------------------------------------------------------------
// DDelegatePropertyBase
// ---------------------------------------------------------------------------

void DDelegatePropertyBase::Serialize(AssetArchive& ar, void* objectPtr)
{
    DELTA_VERIFY(objectPtr != nullptr);

    void* fieldAddr = GetValue(objectPtr);
    FDynamicMulticastDelegate* delegate = GetDelegate(fieldAddr);
    SerializeDelegateBindingArray(ar, GetName(), true, delegate, &m_unresolvedBindings, fieldAddr);
}

void DDelegatePropertyBase::SerializeElement(AssetArchive& ar, void* elementAddr)
{
    DELTA_VERIFY(elementAddr != nullptr);

    FDynamicMulticastDelegate* delegate = GetDelegate(elementAddr);
    SerializeDelegateBindingArray(ar, {}, false, delegate, &m_unresolvedBindings, elementAddr);
}

void DDelegatePropertyBase::SetUnresolvedBindings(
    void* fieldAddr,
    std::vector<FUnresolvedDelegateBinding> bindings)
{
    m_unresolvedBindings[fieldAddr] = std::move(bindings);
}

const std::vector<FUnresolvedDelegateBinding>* DDelegatePropertyBase::GetUnresolvedBindings(
    void* fieldAddr) const
{
    auto it = m_unresolvedBindings.find(fieldAddr);
    return (it != m_unresolvedBindings.end()) ? &it->second : nullptr;
}

// ---------------------------------------------------------------------------
// DDelegateProperty
// ---------------------------------------------------------------------------

DDelegateProperty::DDelegateProperty(std::string name, uint32_t offset)
    : DDelegatePropertyBase(std::move(name), "FDynamicMulticastDelegate", offset, sizeof(FDynamicMulticastDelegate))
{
}

void DDelegateProperty::InitializeValue(void* address) const
{
    new (address) FDynamicMulticastDelegate();
}

void DDelegateProperty::DestroyValue(void* address) const
{
    static_cast<FDynamicMulticastDelegate*>(address)->~FDynamicMulticastDelegate();
}

void DDelegateProperty::SetValue(void* instance, const void* field_value) const
{
    void* addr = static_cast<uint8_t*>(instance) + m_offset;
    *static_cast<FDynamicMulticastDelegate*>(addr) =
        *static_cast<const FDynamicMulticastDelegate*>(field_value);
}

void* DDelegateProperty::GetValue(const void* instance) const
{
    return static_cast<uint8_t*>(const_cast<void*>(instance)) + m_offset;
}

void DDelegateProperty::CopyValue(void* dest, const void* src) const
{
    new (dest) FDynamicMulticastDelegate(*GetDelegate(src));
}

bool DDelegateProperty::Identical(const void* a, const void* b) const
{
    const FDynamicMulticastDelegate* lhs = GetDelegate(a);
    const FDynamicMulticastDelegate* rhs = GetDelegate(b);
    return lhs->GetBindings() == rhs->GetBindings();
}

std::string DDelegateProperty::ToString(const void* address) const
{
    const FDynamicMulticastDelegate* delegate = GetDelegate(address);
    return "Delegate(" + std::to_string(delegate->GetBindingCount()) + " bindings)";
}

EPropertyType DDelegateProperty::GetPropertyType() const
{
    return EPropertyType::Delegate;
}

void DDelegateProperty::ResolveBindings(
    void* fieldAddr,
    const std::function<DObject*(const ScriptPointer&)>& resolve)
{
    auto it = m_unresolvedBindings.find(fieldAddr);
    if (it == m_unresolvedBindings.end())
        return;

    FDynamicMulticastDelegate* delegate = GetDelegate(fieldAddr);
    delegate->Clear();

    for (const FUnresolvedDelegateBinding& binding : it->second)
    {
        DObject* object = resolve(binding.m_object);
        if (object)
            delegate->AddDynamic(object, binding.m_functionName);
    }

    m_unresolvedBindings.erase(it);
}

FDynamicMulticastDelegate* DDelegateProperty::GetDelegate(void* fieldAddr)
{
    return static_cast<FDynamicMulticastDelegate*>(fieldAddr);
}

const FDynamicMulticastDelegate* DDelegateProperty::GetDelegate(const void* fieldAddr) const
{
    return static_cast<const FDynamicMulticastDelegate*>(fieldAddr);
}
