#pragma once

#include "EngineIncludes.h"

#include <string>
#include <vector>

#include <assimp/material.h>

#include "Graphics/Structures/Vertex.h"
#include "Runtime/Core/DObject.h"
#include "Serialization/ISerializationCallbackReceiver.h"
#include "Serialization/TBulkData.h"

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

    /// Loads mesh data from the given source asset path.
    DELTAENGINE_API void Initialize(std::wstring sourcePath);

    /// Imports geometry from an absolute file path (skips engine source asset prefix and texture loading).
    DELTAENGINE_API void ImportFromAbsolutePath(std::wstring absolutePath);

    /// Replaces the material list for this mesh.
    DFUNCTION()
    DELTAENGINE_API void SetMaterials(std::vector<DMaterial*>& materials);

    const std::wstring& GetSourcePath() const { return m_sourcePath; }

    /// Imports mesh geometry and textures from the source path.
    void ImportMesh();

    /// Processes an assimp node and its children.
    void ProcessNode(aiNode* node, const aiScene* scene, DirectX::XMMATRIX accTransform, const std::string& absolutePath, bool loadTextures);
    /// Processes a single assimp mesh into runtime geometry buffers.
    void ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::string& absolutePath, bool loadTextures);
    /// Loads textures for a material slot from the imported scene.
    std::vector<DTexture*> LoadMaterialTextures(const aiScene* scene, aiMaterial* mat, aiTextureType type, std::string typeName, const std::string& filePath);

    void ImportMeshImpl(const std::wstring& absolutePath, bool loadTextures);

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

    /// Packs runtime geometry into bulk-data fields before serialization.
    void OnBeforeSerialize() override;
    /// Restores runtime geometry from bulk-data fields after deserialization.
    void OnAfterDeserialize() override;

private:
    std::vector<std::vector<Vertex>> m_vertices;
    std::vector<std::vector<unsigned int>> m_indices;

    DPROPERTY()
    TBulkData m_vertexData;

    DPROPERTY()
    TBulkData m_indexData;

    DPROPERTY()
    std::vector<DMaterial*> m_materials;

    DPROPERTY()
    std::vector<DTexture*> m_textures;

    DPROPERTY()
    std::wstring m_sourcePath;
};

DELTA_ENGINE_NS_END
