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
    DMesh(std::wstring sourcePath);
    DMesh(std::wstring sourcePath, std::shared_ptr<DMaterial> material);
    //DMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, std::shared_ptr<DMaterial>& material);

    DFUNCTION()
    void SetMaterials(std::vector<std::shared_ptr<DMaterial>>& materials) { m_materials = materials; }

    void ImportMesh();

    void ProcessNode(aiNode* node, const aiScene* scene, DirectX::XMMATRIX accTransform);
    void ProcessMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<std::shared_ptr<DTexture>> LoadMaterialTextures(const aiScene* scene, aiMaterial* mat, aiTextureType type, std::string typeName, const std::string& filePath);

    DFUNCTION()
    std::shared_ptr<DMaterial> GetMaterial(int index = 0) const;

    DFUNCTION()
    inline const std::vector<std::vector<Vertex>>& GetVertices() const { return m_vertices; }
    DFUNCTION()
    inline const std::vector<std::vector<unsigned int>>& GetIndices() const { return m_indices; }
    DFUNCTION()
    inline const std::vector<std::shared_ptr<DMaterial>>& GetMaterials() const { return m_materials; }
    DFUNCTION()
    inline const std::vector<std::shared_ptr<DTexture>>& GetTextures() const { return m_textures; }
    DFUNCTION()
    inline int GetSubMeshCount() const { return static_cast<int>(m_vertices.size()); }

private:
    DPROPERTY()
    std::vector<std::vector<Vertex>> m_vertices;
    DPROPERTY()
    std::vector<std::vector<unsigned int>> m_indices;
    DPROPERTY()
    std::vector<std::shared_ptr<DMaterial>> m_materials;
    DPROPERTY()
    std::vector<std::shared_ptr<DTexture>> m_textures;
    DPROPERTY()
    std::wstring m_sourcePath;
};

DELTA_ENGINE_NS_END
