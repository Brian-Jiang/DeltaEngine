#pragma once

#include "EngineIncludes.h"

#include <vector>
#include <memory>

#include "Core/DTexture.h"
#include "Graphics/Structures/Vertex.h"
#include "Runtime/Core/DObject.h"
#include "assimp/material.h"

struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;

DELTA_ENGINE_NS_BEGIN

class DMaterial;

class DMesh : public DObject
{
public:
    DMesh();
    DMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, std::shared_ptr<DMaterial>& material);

    void ImportMesh();
    void ProcessNode(aiNode* node, const aiScene* scene, DirectX::XMMATRIX accTransform);
    void ProcessMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<std::shared_ptr<DTexture>> LoadMaterialTextures(const aiScene* scene, aiMaterial* mat, aiTextureType type, std::string typeName, const std::string& filePath);

    inline const std::vector<Vertex>& GetVertices() const { return m_vertices; }
    inline const std::vector<unsigned int>& GetIndices() const { return m_indices; }
    inline std::shared_ptr<DMaterial> GetMaterial() const { return m_material; }

private:
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
    std::shared_ptr<DMaterial> m_material;
    std::wstring m_sourcePath;
};

DELTA_ENGINE_NS_END
