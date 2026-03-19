#pragma once

#include "EngineIncludes.h"

#include "Assets/DPrimaryAsset.h"

#include "PA_CommonAssets.generated.h"

DELTA_ENGINE_NS_BEGIN

class DShader;
class DMaterial;
class DTexture;
class DMesh;

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
};

DCLASS()
class DELTAENGINE_API PA_StaticMesh : public DPrimaryAsset
{
    DGENERATED_BODY(PA_StaticMesh)

public:
    static PA_StaticMesh* Create(DMesh* mesh);
    DMesh* GetStaticMesh() const;
};

DELTA_ENGINE_NS_END
