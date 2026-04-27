#include "Editor/Commands/PropertyValueIO.h"
#include "Editor/Commands/EditorCommand.h"
#include "Editor/EditorCore.h"

#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/DVectorProperty.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/DObject.h"

#include "SimpleMath.h"
#include <DirectXMath.h>

using namespace DeltaEngine;
using namespace DirectX;
using namespace DirectX::SimpleMath;

nlohmann::json DeltaEngine::PropertyToJson(const DObject* obj, const DProperty* prop)
{
    const void* addr = prop->GetValue(obj);

    switch (prop->GetPropertyType())
    {
    case EPropertyType::Float:
        return *static_cast<const float*>(addr);
    case EPropertyType::Int:
        return *static_cast<const int*>(addr);
    case EPropertyType::Bool:
        return *static_cast<const bool*>(addr);
    case EPropertyType::Double:
        return *static_cast<const double*>(addr);
    case EPropertyType::String:
        return *static_cast<const std::string*>(addr);
    case EPropertyType::Vector3:
    {
        const auto& v = *static_cast<const Vector3*>(addr);
        return nlohmann::json::array({ v.x, v.y, v.z });
    }
    case EPropertyType::Quaternion:
    {
        const auto& q = *static_cast<const Quaternion*>(addr);
        return nlohmann::json::array({ q.x, q.y, q.z, q.w });
    }
    case EPropertyType::Float4:
    {
        const auto& f = *static_cast<const XMFLOAT4*>(addr);
        return nlohmann::json::array({ f.x, f.y, f.z, f.w });
    }
    case EPropertyType::Float4x4:
    {
        const auto& m = *static_cast<const XMFLOAT4X4*>(addr);
        nlohmann::json arr = nlohmann::json::array();
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                arr.push_back(m.m[r][c]);
        return arr;
    }
    // todo other types
    default:
        DLOG(LogEditorCommand, ELogLevel::Warning, "[PropertyValueIO] PropertyToJson: unsupported type for '{}'", prop->GetName());
        return nullptr;
    }
}

bool DeltaEngine::SetPropertyFromJson(DObject* obj, const DProperty* prop, const nlohmann::json& value)
{
    void* addr = reinterpret_cast<char*>(obj) + prop->GetOffset();

    switch (prop->GetPropertyType())
    {
    case EPropertyType::Float:
        *static_cast<float*>(addr) = value.get<float>();
        break;
    case EPropertyType::Int:
        *static_cast<int*>(addr) = value.get<int>();
        break;
    case EPropertyType::Bool:
        *static_cast<bool*>(addr) = value.get<bool>();
        break;
    case EPropertyType::Double:
        *static_cast<double*>(addr) = value.get<double>();
        break;
    case EPropertyType::String:
        *static_cast<std::string*>(addr) = value.get<std::string>();
        break;
    case EPropertyType::Vector3:
    {
        auto& v = *static_cast<Vector3*>(addr);
        v.x = value[0].get<float>();
        v.y = value[1].get<float>();
        v.z = value[2].get<float>();
        break;
    }
    case EPropertyType::Quaternion:
    {
        auto& q = *static_cast<Quaternion*>(addr);
        q.x = value[0].get<float>();
        q.y = value[1].get<float>();
        q.z = value[2].get<float>();
        q.w = value[3].get<float>();
        break;
    }
    case EPropertyType::Float4:
    {
        auto& f = *static_cast<XMFLOAT4*>(addr);
        f.x = value[0].get<float>();
        f.y = value[1].get<float>();
        f.z = value[2].get<float>();
        f.w = value[3].get<float>();
        break;
    }
    case EPropertyType::Float4x4:
    {
        auto& m = *static_cast<XMFLOAT4X4*>(addr);
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                m.m[r][c] = value[static_cast<size_t>(r * 4 + c)].get<float>();
        break;
    }
    default:
        DLOG(LogEditorCommand, ELogLevel::Error, "[PropertyValueIO] SetPropertyFromJson: unsupported type for '{}'", prop->GetName());
        return false;
    }

    obj->MarkDirty();
    obj->PostEditChangeProperty(prop);
    return true;
}

nlohmann::json DeltaEngine::PropertyToJson(const DObject* obj, const DProperty* prop, EditorCore& /*core*/)
{
    if (prop->GetPropertyType() == EPropertyType::ObjectPtr)
    {
        DObject* pointed = prop->GetObjectPointer(obj);
        if (!pointed || !pointed->GetOwningAsset())
            return nullptr;
        return nlohmann::json{
            {"assetId",  pointed->GetOwningAsset()->GetAssetId().ToString()},
            {"objectId", pointed->GetObjectId().ToString()}
        };
    }
    if (prop->GetPropertyType() == EPropertyType::Vector)
    {
        const auto* vecProp = static_cast<const DVectorPropertyBase*>(prop);
        const DProperty* inner = vecProp->GetInnerProperty();
        if (!inner || inner->GetPropertyType() != EPropertyType::ObjectPtr)
            return nullptr;
        const size_t count = vecProp->GetSize(obj);
        nlohmann::json arr = nlohmann::json::array();
        for (size_t i = 0; i < count; ++i)
        {
            void* elemAddr = vecProp->GetElementAddress(const_cast<DObject*>(obj), i);
            DObject* pointed = inner->GetObjectPointer(elemAddr);
            if (!pointed || !pointed->GetOwningAsset())
                arr.push_back(nullptr);
            else
                arr.push_back({
                    {"assetId",  pointed->GetOwningAsset()->GetAssetId().ToString()},
                    {"objectId", pointed->GetObjectId().ToString()}
                });
        }
        return arr;
    }
    return PropertyToJson(obj, prop);
}

bool DeltaEngine::SetPropertyFromJson(DObject* obj, const DProperty* prop, const nlohmann::json& value, EditorCore& core)
{
    if (prop->GetPropertyType() == EPropertyType::ObjectPtr)
    {
        auto* ptrProp = const_cast<DObjectPtrPropertyBase*>(
            static_cast<const DObjectPtrPropertyBase*>(prop));
        if (value.is_null())
        {
            ptrProp->ResolvePointer(obj, nullptr);
        }
        else
        {
            AssetId  assetId  = AssetId::FromString(value["assetId"].get<std::string>());
            ObjectId objectId = ObjectId::FromString(value["objectId"].get<std::string>());
            DObject* target   = core.ResolveObject(assetId, objectId);
            ptrProp->ResolvePointer(obj, target);
        }
        obj->MarkDirty();
        obj->PostEditChangeProperty(prop);
        return true;
    }
    if (prop->GetPropertyType() == EPropertyType::Vector)
    {
        const auto* vecProp = static_cast<const DVectorPropertyBase*>(prop);
        const DProperty* inner = vecProp->GetInnerProperty();
        if (!inner || inner->GetPropertyType() != EPropertyType::ObjectPtr)
            return SetPropertyFromJson(obj, prop, value);
        if (!value.is_array())
            return false;
        auto* ptrProp = const_cast<DObjectPtrPropertyBase*>(
            static_cast<const DObjectPtrPropertyBase*>(inner));
        const size_t count = std::min(value.size(), vecProp->GetSize(obj));
        for (size_t i = 0; i < count; ++i)
        {
            void* elemAddr = vecProp->GetElementAddress(obj, i);
            const nlohmann::json& elem = value[i];
            if (elem.is_null())
            {
                ptrProp->ResolvePointer(elemAddr, nullptr);
            }
            else
            {
                AssetId  aId = AssetId::FromString(elem["assetId"].get<std::string>());
                ObjectId oId = ObjectId::FromString(elem["objectId"].get<std::string>());
                DObject* target = core.ResolveObject(aId, oId);
                ptrProp->ResolvePointer(elemAddr, target);
            }
        }
        obj->MarkDirty();
        obj->PostEditChangeProperty(prop);
        return true;
    }
    return SetPropertyFromJson(obj, prop, value);
}
