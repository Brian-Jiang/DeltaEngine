#include "Editor/Commands/PropertyValueIO.h"

#include "Editor/Commands/EditorCommand.h"
#include "Editor/EditorCore.h"

#include "Runtime/Reflection/DClass.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/DObject.h"

#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/DStruct.h"
#include "Runtime/Reflection/DVectorProperty.h"

#include "SimpleMath.h"
#include <DirectXMath.h>

#include <filesystem>

using namespace DeltaEngine;
using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace
{

nlohmann::json StructFieldsToJson(void* basePtr, DStruct* ds)
{
    nlohmann::json j = nlohmann::json::object();
    if (!ds || !basePtr)
        return j;

    if (DStruct* sup = ds->GetSuper())
    {
        nlohmann::json superJ = StructFieldsToJson(basePtr, sup);
        if (superJ.is_object())
            j.update(std::move(superJ));
    }

    for (DProperty* prop = ds->GetOwnProperties(); prop; prop = prop->GetNext())
    {
        switch (prop->GetPropertyType())
        {
        case EPropertyType::Float:
            j[prop->GetName()] = *static_cast<float*>(prop->GetValue(basePtr));
            break;
        case EPropertyType::Int:
            j[prop->GetName()] = *static_cast<int*>(prop->GetValue(basePtr));
            break;
        case EPropertyType::Bool:
            j[prop->GetName()] = *static_cast<bool*>(prop->GetValue(basePtr));
            break;
        case EPropertyType::Double:
            j[prop->GetName()] = *static_cast<double*>(prop->GetValue(basePtr));
            break;
        case EPropertyType::String:
            j[prop->GetName()] = *static_cast<std::string*>(prop->GetValue(basePtr));
            break;
        case EPropertyType::FilesystemPath:
            j[prop->GetName()] = prop->ToString(prop->GetValue(basePtr));
            break;
        case EPropertyType::Struct:
        {
            auto* dsp = static_cast<DStructProperty*>(prop);
            void* nestedBase = dsp->GetValue(basePtr);
            DStruct* nestedSchema = dsp->GetSchema();
            j[prop->GetName()] = StructFieldsToJson(nestedBase, nestedSchema);
            break;
        }
        default:
            DLOG(LogEditorCommand, ELogLevel::Warning,
                 "[PropertyValueIO] StructFieldsToJson skipped nested property '{}' "
                 "(EPropertyType not supported for nested struct serialization in this helper)",
                 prop->GetName());
            break;
        }
    }

    return j;
}

bool SetStructFieldsFromJson(void* basePtr, DStruct* ds, const nlohmann::json& value)
{
    if (!ds || !basePtr || !value.is_object())
        return false;

    bool ok = true;

    if (DStruct* sup = ds->GetSuper())
        ok = SetStructFieldsFromJson(basePtr, sup, value) && ok;

    for (DProperty* prop = ds->GetOwnProperties(); prop; prop = prop->GetNext())
    {
        const auto it = value.find(prop->GetName());
        if (it == value.end())
            continue;

        void* addr = prop->GetValue(basePtr);

        switch (prop->GetPropertyType())
        {
        case EPropertyType::Float:
            *static_cast<float*>(addr) = it->get<float>();
            break;
        case EPropertyType::Int:
            *static_cast<int*>(addr) = it->get<int>();
            break;
        case EPropertyType::Bool:
            *static_cast<bool*>(addr) = it->get<bool>();
            break;
        case EPropertyType::Double:
            *static_cast<double*>(addr) = it->get<double>();
            break;
        case EPropertyType::String:
            *static_cast<std::string*>(addr) = it->get<std::string>();
            break;
        case EPropertyType::FilesystemPath:
        {
            const std::string s = it->get<std::string>();
            *static_cast<std::filesystem::path*>(addr) =
                std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(s.data()), s.size()));
            break;
        }
        case EPropertyType::Struct:
        {
            auto* dsp = static_cast<DStructProperty*>(prop);
            void* nestedBase = dsp->GetValue(basePtr);
            DStruct* nestedSchema = dsp->GetSchema();
            if (!SetStructFieldsFromJson(nestedBase, nestedSchema, *it))
                ok = false;
            break;
        }
        default:
            ok = false;
            DLOG(LogEditorCommand, ELogLevel::Error,
                 "[PropertyValueIO] SetStructFieldsFromJson cannot apply nested property '{}' "
                 "(unsupported EPropertyType for JSON write)",
                 prop->GetName());
            break;
        }
    }

    return ok;
}

}

nlohmann::json DeltaEngine::PropertyToJson(const DObject* obj, const DProperty* prop)
{
    if (!obj)
    {
        DLOG(LogEditorCommand, ELogLevel::Warning,
             "[PropertyValueIO] PropertyToJson: null object (expected valid DObject* for serialization)");
        return nlohmann::json(nullptr);
    }
    if (!prop)
    {
        DLOG(LogEditorCommand, ELogLevel::Warning,
             "[PropertyValueIO] PropertyToJson: null property (expected valid DProperty* for '{}')",
             obj->GetClass() ? obj->GetClass()->GetName() : "(null class)");
        return nlohmann::json(nullptr);
    }

    try
    {
        if (prop->GetPropertyType() == EPropertyType::Struct)
        {
            auto* dsp = static_cast<const DStructProperty*>(prop);
            DStruct* schema = dsp->GetSchema();
            void* structBase = dsp->GetValue(const_cast<DObject*>(obj));
            return StructFieldsToJson(structBase, schema);
        }

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
        case EPropertyType::FilesystemPath:
            return prop->ToString(addr);
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
            DLOG(LogEditorCommand, ELogLevel::Warning,
                 "[PropertyValueIO] PropertyToJson: unsupported scalar type '{}' (property '{}' on class '{}')",
                 static_cast<int>(prop->GetPropertyType()),
                 prop->GetName(),
                 obj->GetClass() ? obj->GetClass()->GetName() : "(null)");
            return nullptr;
        }
    }
    catch (const std::exception& e)
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[PropertyValueIO] PropertyToJson threw for '{}' on object class '{}': {} "
             "(expected serializable primitive or struct subset)",
             prop->GetName(),
             obj->GetClass() ? obj->GetClass()->GetName() : "(null)",
             e.what());
        return nlohmann::json(nullptr);
    }
}

bool DeltaEngine::SetPropertyFromJson(DObject* obj, const DProperty* prop, const nlohmann::json& value)
{
    if (!obj)
    {
        DLOG(LogEditorCommand, ELogLevel::Warning,
             "[PropertyValueIO] SetPropertyFromJson: null object (cannot write JSON to instance)");
        return false;
    }
    if (!prop)
    {
        DLOG(LogEditorCommand, ELogLevel::Warning,
             "[PropertyValueIO] SetPropertyFromJson: null property (no reflected field to assign)");
        return false;
    }

    try
    {
        if (prop->GetPropertyType() == EPropertyType::Struct)
        {
            auto* dsp = static_cast<const DStructProperty*>(prop);
            DStruct* schema = dsp->GetSchema();
            void* structBase = dsp->GetValue(obj);
            if (!SetStructFieldsFromJson(structBase, schema, value))
                return false;
            obj->MarkDirty();
            obj->PostEditChangeProperty(prop);
            return true;
        }

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
        case EPropertyType::FilesystemPath:
        {
            const std::string s = value.get<std::string>();
            *static_cast<std::filesystem::path*>(addr) =
                std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(s.data()), s.size()));
            break;
        }
        case EPropertyType::Vector3:
        {
            auto& v = *static_cast<Vector3*>(addr);
            v.x = value.at(0).get<float>();
            v.y = value.at(1).get<float>();
            v.z = value.at(2).get<float>();
            break;
        }
        case EPropertyType::Quaternion:
        {
            auto& q = *static_cast<Quaternion*>(addr);
            q.x = value.at(0).get<float>();
            q.y = value.at(1).get<float>();
            q.z = value.at(2).get<float>();
            q.w = value.at(3).get<float>();
            break;
        }
        case EPropertyType::Float4:
        {
            auto& f = *static_cast<XMFLOAT4*>(addr);
            f.x = value.at(0).get<float>();
            f.y = value.at(1).get<float>();
            f.z = value.at(2).get<float>();
            f.w = value.at(3).get<float>();
            break;
        }
        case EPropertyType::Float4x4:
        {
            auto& m = *static_cast<XMFLOAT4X4*>(addr);
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    m.m[r][c] = value.at(static_cast<size_t>(r * 4 + c)).get<float>();
            break;
        }
        default:
            DLOG(LogEditorCommand, ELogLevel::Error,
                 "[PropertyValueIO] SetPropertyFromJson: unsupported scalar type discriminator {} "
                 "(property '{}' on class '{}') — cannot coerce JSON into storage",
                 static_cast<int>(prop->GetPropertyType()),
                 prop->GetName(),
                 obj->GetClass() ? obj->GetClass()->GetName() : "(null)");
            return false;
        }

        obj->MarkDirty();
        obj->PostEditChangeProperty(prop);
        return true;
    }
    catch (const std::exception& e)
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[PropertyValueIO] SetPropertyFromJson threw for '{}' on class '{}': {} "
             "(JSON shape or element type mismatched reflected property)",
             prop->GetName(),
             obj->GetClass() ? obj->GetClass()->GetName() : "(null)",
             e.what());
        return false;
    }
}

nlohmann::json DeltaEngine::PropertyToJson(const DObject* obj, const DProperty* prop, EditorCore& /*core*/)
{
    if (!obj)
    {
        DLOG(LogEditorCommand, ELogLevel::Warning,
             "[PropertyValueIO] PropertyToJson(EditorCore): null object");
        return nlohmann::json(nullptr);
    }
    if (!prop)
    {
        DLOG(LogEditorCommand, ELogLevel::Warning,
             "[PropertyValueIO] PropertyToJson(EditorCore): null property");
        return nlohmann::json(nullptr);
    }

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
        void* vecStorage = vecProp->GetValue(const_cast<DObject*>(obj));
        const size_t count = vecProp->GetSize(vecStorage);
        nlohmann::json arr = nlohmann::json::array();
        for (size_t i = 0; i < count; ++i)
        {
            void* elemAddr = vecProp->GetElementAddress(vecStorage, i);
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
    if (!obj)
    {
        DLOG(LogEditorCommand, ELogLevel::Warning,
             "[PropertyValueIO] SetPropertyFromJson(EditorCore): null object");
        return false;
    }
    if (!prop)
    {
        DLOG(LogEditorCommand, ELogLevel::Warning,
             "[PropertyValueIO] SetPropertyFromJson(EditorCore): null property");
        return false;
    }

    try
    {
        if (prop->GetPropertyType() == EPropertyType::ObjectPtr)
        {
            auto* ptrProp = const_cast<DObjectPtrPropertyBase*>(
                static_cast<const DObjectPtrPropertyBase*>(prop));
            if (value.is_null())
                ptrProp->ResolvePointer(ptrProp->GetValue(obj), nullptr);
            else
            {
                if (!value.is_object())
                {
                    DLOG(LogEditorCommand, ELogLevel::Error,
                         "[PropertyValueIO] SetPropertyFromJson ObjectPtr: expected JSON object with "
                         "string keys 'assetId' and 'objectId' but got discriminator {}",
                         value.type_name());
                    return false;
                }
                const std::string assetStr = value.at("assetId").get<std::string>();
                const std::string objectStr = value.at("objectId").get<std::string>();
                const AssetId assetId = AssetId::FromString(assetStr);
                const ObjectId objectId = ObjectId::FromString(objectStr);
                DObject* target = core.ResolveObject(assetId, objectId);
                ptrProp->ResolvePointer(ptrProp->GetValue(obj), target);
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
            {
                DLOG(LogEditorCommand, ELogLevel::Error,
                     "[PropertyValueIO] ObjectPtr vector JSON must be array (got {}) — property '{}'",
                     value.type_name(),
                     prop->GetName());
                return false;
            }

            auto* ptrProp = const_cast<DObjectPtrPropertyBase*>(
                static_cast<const DObjectPtrPropertyBase*>(inner));
            void* vecStorage = vecProp->GetValue(obj);
            const size_t count = std::min(value.size(), vecProp->GetSize(vecStorage));
            for (size_t i = 0; i < count; ++i)
            {
                void* elemAddr = vecProp->GetElementAddress(vecStorage, i);
                const nlohmann::json& elem = value[i];
                if (elem.is_null())
                    ptrProp->ResolvePointer(elemAddr, nullptr);
                else
                {
                    const std::string assetStr = elem.at("assetId").get<std::string>();
                    const std::string objectStr = elem.at("objectId").get<std::string>();
                    AssetId aId = AssetId::FromString(assetStr);
                    ObjectId oId = ObjectId::FromString(objectStr);
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
    catch (const std::exception& e)
    {
        DLOG(LogEditorCommand, ELogLevel::Error,
             "[PropertyValueIO] SetPropertyFromJson(EditorCore&) threw on property '{}': {} "
             "(ObjectPtr payloads require {{assetId, objectId}} objects or array thereof)",
             prop->GetName(),
             e.what());
        return false;
    }
}
