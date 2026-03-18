#pragma once

#include "EngineIncludes.h"

#include <vector>
#include <memory>

#include "Core/DTexture.h"
#include "Graphics/Structures/Vertex.h"
#include "Runtime/Core/DObject.h"
#include "assimp/material.h"

#include "DMesh.generated.h"

struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;

DELTA_ENGINE_NS_BEGIN

class DMaterial;

DCLASS()
class DMesh : public DObject
{
    DGENERATED_BODY(DMesh)

public:
    DMesh();
    //DMesh(std::wstring sourcePath);
    //DMesh(std::wstring sourcePath, std::shared_ptr<DMaterial> material);
    //DMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, std::shared_ptr<DMaterial>& material);

    DELTAENGINE_API void Initialize(std::wstring sourcePath);

    DFUNCTION()
    DELTAENGINE_API void SetMaterials(std::vector<DMaterial*>& materials);

    void ImportMesh();

    void ProcessNode(aiNode* node, const aiScene* scene, DirectX::XMMATRIX accTransform);
    void ProcessMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<DTexture*> LoadMaterialTextures(const aiScene* scene, aiMaterial* mat, aiTextureType type, std::string typeName, const std::string& filePath);

    DFUNCTION()
    DELTAENGINE_API DMaterial* GetMaterial(int index = 0) const;

    DFUNCTION()
    DELTAENGINE_API const std::vector<std::vector<Vertex>>& GetVertices() const;
    DFUNCTION()
    DELTAENGINE_API const std::vector<std::vector<unsigned int>>& GetIndices() const;
    DFUNCTION()
    DELTAENGINE_API const std::vector<DMaterial*>& GetMaterials() const;
    DFUNCTION()
    DELTAENGINE_API const std::vector<DTexture*>& GetTextures() const;
    DFUNCTION()
    DELTAENGINE_API int GetSubMeshCount() const;

private:
    std::vector<std::vector<Vertex>> m_vertices;
    std::vector<std::vector<unsigned int>> m_indices;
    std::vector<DMaterial*> m_materials;
    std::vector<DTexture*> m_textures;

    DPROPERTY()
    std::wstring m_sourcePath;

public:
    DPROPERTY()
    std::vector<float> m_testFloats;
};

DELTA_ENGINE_NS_END
