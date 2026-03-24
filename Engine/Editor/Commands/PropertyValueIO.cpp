#include "Editor/Commands/PropertyValueIO.h"

#include "Runtime/Reflection/DProperty.h"
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
    default:
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
        return false;
    }

    obj->MarkDirty();
    obj->PostEditChangeProperty(prop);
    return true;
}
