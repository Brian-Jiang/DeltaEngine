#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/Structures/Vertex.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Serialization/ISerializationCallbackReceiver.h"
#include "Runtime/Serialization/TBulkData.h"

#include <assimp/material.h>

#include <filesystem>
#include <string>
#include <vector>

#include "DMesh.generated.h"

struct aiNode;
struct aiScene;
struct aiMesh;
struct aiMaterial;

DELTA_ENGINE_NS_BEGIN

class DMaterial;
class DTexture;

DCLASS()
class DMesh : public DObject, public ISerializationCallbackReceiver
{
    DGENERATED_BODY(DMesh)

public:
    DELTAENGINE_API DMesh();
    DELTAENGINE_API ~DMesh();

    DELTAENGINE_API void Initialize(const std::filesystem::path& sourcePath);

    /// Replaces all geometry with a single submesh built from raw vertices and indices.
    DELTAENGINE_API void SetGeometry(std::vector<Vertex> vertices, std::vector<unsigned int> indices);

    DELTAENGINE_API void ImportFromAbsolutePath(const std::filesystem::path& absolutePath);

    DFUNCTION()
    DELTAENGINE_API void SetMaterials(std::vector<DMaterial*>& materials);

    const std::filesystem::path& GetSourcePath() const { return m_sourcePath; }

    void ImportMesh();

    void ProcessNode(aiNode* node, const aiScene* scene, DirectX::XMMATRIX accTransform, const std::filesystem::path& absolutePath, bool loadTextures);
    void ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::filesystem::path& absolutePath, bool loadTextures);
    std::vector<DTexture*> LoadMaterialTextures(const aiScene* scene, aiMaterial* mat, aiTextureType type, std::string typeName, const std::filesystem::path& filePath);

    void ImportMeshImpl(const std::filesystem::path& absolutePath, bool loadTextures);

    /// Returns the material at the requested submesh index, or nullptr.
    DFUNCTION()
    DELTAENGINE_API DMaterial* GetMaterial(int index = 0) const;

    /// Returns the per-submesh vertex buffers.
    DFUNCTION()
    DELTAENGINE_API const std::vector<std::vector<Vertex>>& GetVertices() const;
    /// Returns the per-submesh index buffers.
    DFUNCTION()
    DELTAENGINE_API const std::vector<std::vector<unsigned int>>& GetIndices() const;
    /// Returns the materials assigned to this mesh.
    DFUNCTION()
    DELTAENGINE_API const std::vector<DMaterial*>& GetMaterials() const;
    /// Returns the textures imported with this mesh.
    DFUNCTION()
    DELTAENGINE_API const std::vector<DTexture*>& GetTextures() const;
    /// Returns the number of imported submeshes.
    DFUNCTION()
    DELTAENGINE_API int GetSubMeshCount() const;

    /// Returns the cached local-space AABB center of a submesh, or the origin if the index is invalid.
    DELTAENGINE_API DirectX::XMFLOAT3 GetSubMeshLocalCenter(int index) const;

    /// Packs runtime geometry into bulk-data fields before serialization.
    void OnBeforeSerialize() override;
    /// Restores runtime geometry from bulk-data fields after deserialization.
    void OnAfterDeserialize() override;

private:
    std::vector<std::vector<Vertex>> m_vertices;
    std::vector<std::vector<unsigned int>> m_indices;

    /// Per-submesh local-space AABB centers, lazily rebuilt from m_vertices when stale.
    mutable std::vector<DirectX::XMFLOAT3> m_subMeshLocalCenters;

    DPROPERTY()
    TBulkData m_vertexData;

    DPROPERTY()
    TBulkData m_indexData;

    DPROPERTY()
    std::vector<DMaterial*> m_materials;

    DPROPERTY()
    std::vector<DTexture*> m_textures;

    DPROPERTY()
    std::filesystem::path m_sourcePath;
};

DELTA_ENGINE_NS_END
