#include "Runtime/Assets/PA_CommonAssets.h"

#include "Runtime/Core/DMaterial.h"
#include "Runtime/Core/DMesh.h"
#include "Runtime/Core/DShader.h"
#include "Runtime/Core/DTexture.h"
#include "Runtime/Core/Skybox.h"
#include "Runtime/Core/UUID.h"

using namespace DeltaEngine;

namespace
{

template <typename TAsset, typename TObject>
TAsset* CreateTypedPrimaryAsset(TObject* object, const char* className)
{
    TAsset* asset = CreateDObject<TAsset>();
    DELTA_VERIFY_MSG(asset != nullptr, "Failed to create primary asset instance for '{}'", className);
    asset->GetHeader().m_persistentId = AssetId::Generate();
    asset->GetHeader().m_className = className;

    if (!object)
    {
        DLOG(LogAsset,
             ELogLevel::Warning,
             "Primary asset '{}' created without payload object (expected creator to supply typed payload)",
             className);
    }
    else
    {
        if (object->GetObjectId().IsNull())
            object->SetObjectId(ObjectId::Generate());
        asset->AddObject(object);
    }

    return asset;
}

template <typename TObject>
TObject* FindTypedObject(const DPrimaryAsset* asset)
{
    if (!asset)
        return nullptr;

    for (DObject* object : asset->GetObjects())
    {
        if (TObject* typedObject = dynamic_cast<TObject*>(object))
            return typedObject;
    }

    return nullptr;
}

} // namespace

PA_Shader* PA_Shader::Create(DShader* shader)
{
    return CreateTypedPrimaryAsset<PA_Shader>(shader, "PA_Shader");
}

DShader* PA_Shader::GetShader() const
{
    return FindTypedObject<DShader>(this);
}

PA_Material* PA_Material::Create(DMaterial* material)
{
    return CreateTypedPrimaryAsset<PA_Material>(material, "PA_Material");
}

DMaterial* PA_Material::GetMaterial() const
{
    return FindTypedObject<DMaterial>(this);
}

PA_Texture* PA_Texture::Create(DTexture* texture)
{
    return CreateTypedPrimaryAsset<PA_Texture>(texture, "PA_Texture");
}

DTexture* PA_Texture::GetTexture() const
{
    return FindTypedObject<DTexture>(this);
}

PA_StaticMesh* PA_StaticMesh::Create(DMesh* mesh)
{
    return CreateTypedPrimaryAsset<PA_StaticMesh>(mesh, "PA_StaticMesh");
}

DMesh* PA_StaticMesh::GetStaticMesh() const
{
    return FindTypedObject<DMesh>(this);
}

PA_Skybox* PA_Skybox::Create(Skybox* skybox)
{
    return CreateTypedPrimaryAsset<PA_Skybox>(skybox, "PA_Skybox");
}

Skybox* PA_Skybox::GetSkybox() const
{
    return FindTypedObject<Skybox>(this);
}
