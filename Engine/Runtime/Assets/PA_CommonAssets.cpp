#include "Runtime/Assets/PA_CommonAssets.h"

#include "Runtime/Core/DMaterial.h"
#include "Runtime/Core/DMesh.h"
#include "Runtime/Core/DShader.h"
#include "Runtime/Core/DTexture.h"
#include "Runtime/Core/Skybox.h"
#include "Runtime/Core/UUID.h"
#include "Runtime/Graphics/Structures/Vertex.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

#include <DirectXMath.h>

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
    PA_StaticMesh* asset = CreateTypedPrimaryAsset<PA_StaticMesh>(mesh, "PA_StaticMesh");
    asset->RebuildStaticMeta();
    return asset;
}

DMesh* PA_StaticMesh::GetStaticMesh() const
{
    return FindTypedObject<DMesh>(this);
}

std::pair<DStruct*, void*> PA_StaticMesh::GetStaticMetaSchema()
{
    return { GetReflectionRegistry().FindStructByName("PA_StaticMesh_StaticMeta"), &m_staticMeta };
}

void PA_StaticMesh::RebuildStaticMeta()
{
    m_staticMeta = PA_StaticMesh_StaticMeta{};

    DMesh* mesh = GetStaticMesh();
    if (!mesh)
        return;

    const auto& submeshVertices = mesh->GetVertices();
    const auto& submeshIndices = mesh->GetIndices();

    size_t totalVertices = 0;
    size_t totalIndices = 0;
    for (const auto& sm : submeshVertices)
        totalVertices += sm.size();
    for (const auto& sm : submeshIndices)
        totalIndices += sm.size();

    m_staticMeta.m_vertexCount = static_cast<int>(totalVertices);
    m_staticMeta.m_indexCount = static_cast<int>(totalIndices);

    bool hasBox = false;
    DirectX::BoundingBox merged{};
    for (const auto& sm : submeshVertices)
    {
        if (sm.empty())
            continue;

        DirectX::BoundingBox sub{};
        DirectX::BoundingBox::CreateFromPoints(
            sub,
            sm.size(),
            reinterpret_cast<const DirectX::XMFLOAT3*>(&sm[0].position),
            sizeof(Vertex));

        if (!hasBox)
        {
            merged = sub;
            hasBox = true;
        }
        else
        {
            DirectX::BoundingBox::CreateMerged(merged, merged, sub);
        }
    }

    if (hasBox)
        m_staticMeta.m_aabb = merged;
}

PA_Skybox* PA_Skybox::Create(Skybox* skybox)
{
    return CreateTypedPrimaryAsset<PA_Skybox>(skybox, "PA_Skybox");
}

Skybox* PA_Skybox::GetSkybox() const
{
    return FindTypedObject<Skybox>(this);
}
