#pragma once

#include "EngineIncludes.h"

#include "Runtime/Assets/DPrimaryAsset.h"

#include <DirectXCollision.h>

#include "PA_CommonAssets.generated.h"

DELTA_ENGINE_NS_BEGIN

class DShader;
class DMaterial;
class DTexture;
class DMesh;
class Skybox;

DSTRUCT()
struct DELTAENGINE_API PA_StaticMesh_StaticMeta
{
    DGENERATED_BODY_STRUCT(PA_StaticMesh_StaticMeta)

    DPROPERTY()
    int m_vertexCount = 0;

    DPROPERTY()
    int m_indexCount = 0;

    DPROPERTY()
    DirectX::BoundingBox m_aabb = {};
};

DSTRUCT()
struct DELTAENGINE_API PA_Texture_StaticMeta
{
    DGENERATED_BODY_STRUCT(PA_Texture_StaticMeta)

    DPROPERTY()
    int m_width = 0;

    DPROPERTY()
    int m_height = 0;

    DPROPERTY()
    int m_mipCount = 0;

    DPROPERTY()
    int m_format = 0;
};

DCLASS()
class DELTAENGINE_API PA_Shader : public DPrimaryAsset
{
    DGENERATED_BODY(PA_Shader)

public:
    static PA_Shader* Create(DShader* shader);
    DShader* GetShader() const;
};

DCLASS()
class DELTAENGINE_API PA_Material : public DPrimaryAsset
{
    DGENERATED_BODY(PA_Material)

public:
    static PA_Material* Create(DMaterial* material);
    DMaterial* GetMaterial() const;
};

DCLASS()
class DELTAENGINE_API PA_Texture : public DPrimaryAsset
{
    DGENERATED_BODY(PA_Texture)

public:
    static PA_Texture* Create(DTexture* texture);
    DTexture* GetTexture() const;

    /// Returns the cached static metadata (width/height/mipCount/format) for this texture asset.
    const PA_Texture_StaticMeta& GetStaticMeta() const { return m_staticMeta; }

    std::pair<DStruct*, void*> GetStaticMetaSchema() override;
    void RebuildStaticMeta() override;

private:
    PA_Texture_StaticMeta m_staticMeta;
};

DCLASS()
class DELTAENGINE_API PA_StaticMesh : public DPrimaryAsset
{
    DGENERATED_BODY(PA_StaticMesh)

public:
    static PA_StaticMesh* Create(DMesh* mesh);
    DMesh* GetStaticMesh() const;

    /// Returns the cached static metadata (vertex/index counts, AABB) for this mesh asset.
    const PA_StaticMesh_StaticMeta& GetStaticMeta() const { return m_staticMeta; }

    std::pair<DStruct*, void*> GetStaticMetaSchema() override;
    void RebuildStaticMeta() override;

private:
    PA_StaticMesh_StaticMeta m_staticMeta;
};

DCLASS()
class DELTAENGINE_API PA_Skybox : public DPrimaryAsset
{
    DGENERATED_BODY(PA_Skybox)

  public:
    static PA_Skybox *Create(Skybox *skybox);
    Skybox *GetSkybox() const;
};

DELTA_ENGINE_NS_END
