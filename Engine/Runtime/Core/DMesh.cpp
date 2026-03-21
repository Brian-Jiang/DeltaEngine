#include "Core/DMesh.h"

#include "Core/DTexture.h"
#include "IO/IOManager.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cstring>
#include <filesystem>
#include <iostream>
#include <utility>

namespace
{
std::string GetParentDirectory(const std::string& filePath, int levelsUp)
{
    std::filesystem::path path(filePath);
    for (int level = 0; level < levelsUp; ++level)
        path = path.parent_path();

    return path.string();
}

std::string FindTextureFile(const std::string& directory, const std::string& fileName)
{
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
    {
        if (entry.is_regular_file() && entry.path().filename() == fileName)
            return entry.path().string();
    }

    return {};
}

DeltaEngine::TBulkData SerializeVertices(const std::vector<std::vector<Vertex>>& vertices)
{
    const uint32_t submeshCount = static_cast<uint32_t>(vertices.size());

    uint64_t totalSize = sizeof(uint32_t) + sizeof(uint32_t) * submeshCount;
    for (const auto& submesh : vertices)
        totalSize += sizeof(Vertex) * submesh.size();

    auto* buffer = new uint8_t[totalSize];
    uint8_t* cursor = buffer;

    std::memcpy(cursor, &submeshCount, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    for (const auto& submesh : vertices)
    {
        const uint32_t count = static_cast<uint32_t>(submesh.size());
        std::memcpy(cursor, &count, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
    }

    for (const auto& submesh : vertices)
    {
        const uint64_t bytes = sizeof(Vertex) * submesh.size();
        if (bytes > 0)
            std::memcpy(cursor, submesh.data(), bytes);
        cursor += bytes;
    }

    DeltaEngine::TBulkData bulk;
    bulk.Set(buffer, totalSize);
    delete[] buffer;
    return bulk;
}

void DeserializeVertices(const DeltaEngine::TBulkData& bulk, std::vector<std::vector<Vertex>>& outVertices)
{
    if (!bulk.IsValid())
        return;

    const uint8_t* cursor = bulk.m_data;

    uint32_t submeshCount = 0;
    std::memcpy(&submeshCount, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    std::vector<uint32_t> counts(submeshCount);
    for (uint32_t index = 0; index < submeshCount; ++index)
    {
        std::memcpy(&counts[index], cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
    }

    outVertices.resize(submeshCount);
    for (uint32_t index = 0; index < submeshCount; ++index)
    {
        outVertices[index].resize(counts[index]);
        const uint64_t bytes = sizeof(Vertex) * counts[index];
        if (bytes > 0)
            std::memcpy(outVertices[index].data(), cursor, bytes);
        cursor += bytes;
    }
}

DeltaEngine::TBulkData SerializeIndices(const std::vector<std::vector<unsigned int>>& indices)
{
    const uint32_t submeshCount = static_cast<uint32_t>(indices.size());

    uint64_t totalSize = sizeof(uint32_t) + sizeof(uint32_t) * submeshCount;
    for (const auto& submesh : indices)
        totalSize += sizeof(unsigned int) * submesh.size();

    auto* buffer = new uint8_t[totalSize];
    uint8_t* cursor = buffer;

    std::memcpy(cursor, &submeshCount, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    for (const auto& submesh : indices)
    {
        const uint32_t count = static_cast<uint32_t>(submesh.size());
        std::memcpy(cursor, &count, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
    }

    for (const auto& submesh : indices)
    {
        const uint64_t bytes = sizeof(unsigned int) * submesh.size();
        if (bytes > 0)
            std::memcpy(cursor, submesh.data(), bytes);
        cursor += bytes;
    }

    DeltaEngine::TBulkData bulk;
    bulk.Set(buffer, totalSize);
    delete[] buffer;
    return bulk;
}

void DeserializeIndices(const DeltaEngine::TBulkData& bulk, std::vector<std::vector<unsigned int>>& outIndices)
{
    if (!bulk.IsValid())
        return;

    const uint8_t* cursor = bulk.m_data;

    uint32_t submeshCount = 0;
    std::memcpy(&submeshCount, cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    std::vector<uint32_t> counts(submeshCount);
    for (uint32_t index = 0; index < submeshCount; ++index)
    {
        std::memcpy(&counts[index], cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
    }

    outIndices.resize(submeshCount);
    for (uint32_t index = 0; index < submeshCount; ++index)
    {
        outIndices[index].resize(counts[index]);
        const uint64_t bytes = sizeof(unsigned int) * counts[index];
        if (bytes > 0)
            std::memcpy(outIndices[index].data(), cursor, bytes);
        cursor += bytes;
    }
}
}

using namespace DeltaEngine;

DMesh::DMesh() = default;

DMesh::~DMesh() = default;

void DMesh::ImportMesh()
{
    m_vertices.clear();
    m_indices.clear();
    m_textures.clear();

    const std::wstring fullPath = IOManager::GetEngineSourceAssetFullPath(m_sourcePath);
    const std::filesystem::path path(fullPath);
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, true);

    constexpr unsigned int kImportFlags =
        aiProcess_SortByPType |
        aiProcess_JoinIdenticalVertices |
        aiProcess_Triangulate |
        aiProcess_GenBoundingBoxes |
        aiProcess_FlipUVs |
        aiProcess_MakeLeftHanded |
        aiProcess_ImproveCacheLocality |
        aiProcess_FlipWindingOrder |
        aiProcess_TransformUVCoords |
        aiProcess_PreTransformVertices;

            //// aiProcess_CalcTangentSpace |
            //// aiProcess_JoinIdenticalVertices |
            //// aiProcess_Triangulate |
            //// aiProcess_RemoveComponent |
            //// aiProcess_GenSmoothNormals |
            //// aiProcess_SplitLargeMeshes |
            //// aiProcess_ValidateDataStructure |
            //////aiProcess_ImproveCacheLocality | // handled by optimizePostTransform()
            //// aiProcess_RemoveRedundantMaterials |
            //aiProcess_SortByPType |
            //// aiProcess_FindInvalidData |
            //// aiProcess_GenUVCoords |
            //// aiProcess_TransformUVCoords |
            //// aiProcess_OptimizeMeshes |
            //// aiProcess_OptimizeGraph;

            //// aiProcess_CalcTangentSpace |
            //aiProcess_JoinIdenticalVertices |
            //aiProcess_Triangulate |
            //// aiProcess_RemoveComponent |
            //// aiProcess_GenSmoothNormals |
            //aiProcess_GenBoundingBoxes |
            //////aiProcess_SplitLargeMeshes |
            //////aiProcess_ValidateDataStructure |
            //aiProcess_FlipUVs | aiProcess_MakeLeftHanded |
            //// aiProcess_ConvertToLeftHanded |
            //aiProcess_ImproveCacheLocality | aiProcess_FlipWindingOrder |
            //// aiProcess_RemoveRedundantMaterials | // remove redundant materials
            //// aiProcess_FindDegenerates | // remove degenerated polygons from the import
            //// aiProcess_FindInvalidData | // detect invalid model data, such as invalid normal vectors
            //// aiProcess_GenUVCoords | // convert spherical, cylindrical, box and planar mapping to proper UVs
            //aiProcess_TransformUVCoords | // preprocess UV transformations (scaling, translation ...)
            //// aiProcess_OptimizeMeshes | // join small meshes, if possible;
            //aiProcess_PreTransformVertices | //-- fixes the transformation issue.
            //0;

    const aiScene* scene = importer.ReadFile(path.string(), kImportFlags);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }

    ProcessNode(scene->mRootNode, scene, DirectX::XMMatrixIdentity());
}

void DMesh::ProcessNode(aiNode* node, const aiScene* scene, DirectX::XMMATRIX accTransform)
{
    const auto localTransformation = DirectX::XMMATRIX(&(node->mTransformation.a1));
    accTransform = DirectX::XMMatrixMultiply(accTransform, localTransformation);

    for (unsigned int meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex)
        ProcessMesh(scene->mMeshes[node->mMeshes[meshIndex]], scene);

    for (unsigned int childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
        ProcessNode(node->mChildren[childIndex], scene, accTransform);
}

void DMesh::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<DTexture*> textures;

    for (unsigned int vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
    {
        Vertex vertex {};
        vertex.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex.position = DirectX::XMFLOAT3(&mesh->mVertices[vertexIndex].x);
        vertex.normal = DirectX::XMFLOAT3(&mesh->mNormals[vertexIndex].x);

        if (mesh->mTextureCoords[0])
        {
            DirectX::XMFLOAT2 uv {};
            uv.x = mesh->mTextureCoords[0][vertexIndex].x;
            uv.y = mesh->mTextureCoords[0][vertexIndex].y;
            vertex.uv = uv;
        }
        else
        {
            vertex.uv = DirectX::XMFLOAT2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
    {
        const aiFace& face = mesh->mFaces[faceIndex];
        for (unsigned int index = 0; index < face.mNumIndices; ++index)
            indices.push_back(face.mIndices[index]);
    }

    if (mesh->mMaterialIndex >= 0)
    {
        const std::wstring fullPath = IOManager::GetEngineSourceAssetFullPath(m_sourcePath);
        const std::filesystem::path path(fullPath);
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::vector<DTexture*> diffuseMaps = LoadMaterialTextures(scene, material, aiTextureType_DIFFUSE, "texture_diffuse", path.string());
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        std::vector<DTexture*> specularMaps = LoadMaterialTextures(scene, material, aiTextureType_SPECULAR, "texture_specular", path.string());
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    }

    m_vertices.push_back(std::move(vertices));
    m_indices.push_back(std::move(indices));
    m_textures.insert(m_textures.end(), textures.begin(), textures.end());
}

std::vector<DTexture*> DMesh::LoadMaterialTextures(const aiScene* scene, aiMaterial* mat, aiTextureType type, std::string typeName, const std::string& filePath)
{
    (void)scene;
    (void)typeName;

    const auto folderPath = GetParentDirectory(filePath, 2);
    std::vector<DTexture*> textures;
    for (unsigned int textureIndex = 0; textureIndex < mat->GetTextureCount(type); ++textureIndex)
    {
        aiString str;
        mat->GetTexture(type, textureIndex, &str);

        const std::string importedPath = str.C_Str();
        if (importedPath.empty())
            continue;

        const auto fileName = importedPath.substr(importedPath.find_last_of("\\/") + 1);
        const auto texturePath = FindTextureFile(folderPath, fileName);
        if (!std::filesystem::exists(texturePath))
            continue;

        DTexture* texture = DTexture::LoadFromFile(std::wstring(texturePath.begin(), texturePath.end()), true);
        textures.push_back(texture);
    }

    return textures;
}

void DMesh::Initialize(std::wstring sourcePath)
{
    m_sourcePath = std::move(sourcePath);
    ImportMesh();
}

void DMesh::SetMaterials(std::vector<DMaterial*>& materials)
{
    m_materials = materials;
}

DMaterial* DMesh::GetMaterial(int index) const
{
    if (index < 0)
        return nullptr;

    const size_t materialIndex = static_cast<size_t>(index);
    return materialIndex < m_materials.size() ? m_materials[materialIndex] : nullptr;
}

const std::vector<std::vector<Vertex>>& DMesh::GetVertices() const { return m_vertices; }

const std::vector<std::vector<unsigned int>>& DMesh::GetIndices() const { return m_indices; }

const std::vector<DMaterial*>& DMesh::GetMaterials() const { return m_materials; }

const std::vector<DTexture*>& DMesh::GetTextures() const { return m_textures; }

int DMesh::GetSubMeshCount() const { return static_cast<int>(m_vertices.size()); }

void DMesh::OnBeforeSerialize()
{
    m_vertexData = SerializeVertices(m_vertices);
    m_indexData = SerializeIndices(m_indices);
}

void DMesh::OnAfterDeserialize()
{
    DeserializeVertices(m_vertexData, m_vertices);
    DeserializeIndices(m_indexData, m_indices);
}
