#include "Runtime/Core/DMesh.h"

#include "Runtime/Core/DTexture.h"
#include "Runtime/IO/IOManager.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cstddef>
#include <cstring>
#include <exception>
#include <filesystem>
#include <utility>

namespace
{
constexpr uint32_t kMaxMeshSubmeshes = 4096u;

std::filesystem::path ParentPathLevelsUp(const std::filesystem::path& path, int levelsUp)
{
    std::filesystem::path p = path;
    for (int level = 0; level < levelsUp; ++level)
        p = p.parent_path();
    return p;
}

std::string PathLog(const std::filesystem::path& p)
{
    const std::u8string u = p.u8string();
    return { reinterpret_cast<const char*>(u.data()), u.size() };
}

std::filesystem::path FindTextureFile(const std::filesystem::path& directory, const std::string& fileName)
{
    try
    {
        if (directory.empty() || !std::filesystem::exists(directory))
            return {};

        const std::filesystem::path wantName(fileName);

        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
        {
            if (entry.is_regular_file() && entry.path().filename() == wantName)
                return entry.path();
        }
    }
    catch (const std::filesystem::filesystem_error& ex)
    {
        DLOG(LogAsset, ELogLevel::Warning,
            "FindTextureFile: filesystem scan failed directory='{}' fileName='{}' — {}",
            PathLog(directory), fileName, ex.what());
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
    outVertices.clear();
    if (!bulk.IsValid())
        return;

    size_t off = 0;
    const uint8_t* base = bulk.m_data;
    const uint64_t bulkSize = bulk.m_size;

    auto consume = [&](void* dst, size_t nbytes, const char* contextTag) -> bool {
        if (off + nbytes > bulkSize)
        {
            DLOG(LogAsset, ELogLevel::Warning,
                "DeserializeVertices: truncated bulk ({}) at offset {} need {} have {}",
                contextTag, static_cast<unsigned long long>(off), static_cast<unsigned long long>(nbytes),
                static_cast<unsigned long long>(bulkSize));
            outVertices.clear();
            return false;
        }
        std::memcpy(dst, base + off, nbytes);
        off += nbytes;
        return true;
    };

    uint32_t submeshCount = 0;
    if (!consume(&submeshCount, sizeof(uint32_t), "submeshCount"))
        return;

    if (submeshCount > kMaxMeshSubmeshes)
    {
        DLOG(LogAsset, ELogLevel::Warning,
            "DeserializeVertices: unreasonable submeshCount {} (max {}) — rejecting bulk",
            submeshCount, kMaxMeshSubmeshes);
        return;
    }

    std::vector<uint32_t> counts(submeshCount);
    for (uint32_t index = 0; index < submeshCount; ++index)
    {
        if (!consume(&counts[index], sizeof(uint32_t), "per-submesh vertex count"))
            return;
    }

    outVertices.resize(submeshCount);
    for (uint32_t index = 0; index < submeshCount; ++index)
    {
        const uint64_t vertBytes64 = static_cast<uint64_t>(counts[index]) * sizeof(Vertex);
        if (vertBytes64 > static_cast<uint64_t>(SIZE_MAX))
        {
            DLOG(LogAsset, ELogLevel::Warning,
                "DeserializeVertices: vertex byte payload overflow submeshIndex={} count={}",
                index, counts[index]);
            outVertices.clear();
            return;
        }
        const size_t vertBytes = static_cast<size_t>(vertBytes64);
        outVertices[index].resize(counts[index]);
        if (vertBytes > 0 && !consume(outVertices[index].data(), vertBytes, "vertex bytes"))
            return;
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
    outIndices.clear();
    if (!bulk.IsValid())
        return;

    size_t off = 0;
    const uint8_t* base = bulk.m_data;
    const uint64_t bulkSize = bulk.m_size;

    auto consume = [&](void* dst, size_t nbytes, const char* contextTag) -> bool {
        if (off + nbytes > bulkSize)
        {
            DLOG(LogAsset, ELogLevel::Warning,
                "DeserializeIndices: truncated bulk ({}) at offset {} need {} have {}",
                contextTag, static_cast<unsigned long long>(off), static_cast<unsigned long long>(nbytes),
                static_cast<unsigned long long>(bulkSize));
            outIndices.clear();
            return false;
        }
        std::memcpy(dst, base + off, nbytes);
        off += nbytes;
        return true;
    };

    uint32_t submeshCount = 0;
    if (!consume(&submeshCount, sizeof(uint32_t), "submeshCount"))
        return;

    if (submeshCount > kMaxMeshSubmeshes)
    {
        DLOG(LogAsset, ELogLevel::Warning,
            "DeserializeIndices: unreasonable submeshCount {} (max {}) — rejecting bulk",
            submeshCount, kMaxMeshSubmeshes);
        return;
    }

    std::vector<uint32_t> counts(submeshCount);
    for (uint32_t index = 0; index < submeshCount; ++index)
    {
        if (!consume(&counts[index], sizeof(uint32_t), "per-submesh index count"))
            return;
    }

    outIndices.resize(submeshCount);
    for (uint32_t index = 0; index < submeshCount; ++index)
    {
        const uint64_t idxBytes64 = static_cast<uint64_t>(counts[index]) * sizeof(unsigned int);
        if (idxBytes64 > static_cast<uint64_t>(SIZE_MAX))
        {
            DLOG(LogAsset, ELogLevel::Warning,
                "DeserializeIndices: index byte payload overflow submeshIndex={} count={}",
                index, counts[index]);
            outIndices.clear();
            return;
        }
        const size_t idxBytes = static_cast<size_t>(idxBytes64);
        outIndices[index].resize(counts[index]);
        if (idxBytes > 0 && !consume(outIndices[index].data(), idxBytes, "index bytes"))
            return;
    }
}
}

using namespace DeltaEngine;

DMesh::DMesh() = default;

DMesh::~DMesh() = default;

void DMesh::ImportMeshImpl(const std::filesystem::path& absolutePath, bool loadTextures)
{
    m_vertices.clear();
    m_indices.clear();
    m_textures.clear();

    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, true);

    constexpr unsigned int kImportFlags =
        aiProcess_SortByPType |
        aiProcess_JoinIdenticalVertices |
        aiProcess_Triangulate |
        aiProcess_GenBoundingBoxes |
        aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs |
        aiProcess_MakeLeftHanded |
        aiProcess_ImproveCacheLocality |
        aiProcess_FlipWindingOrder |
        aiProcess_TransformUVCoords |
        aiProcess_PreTransformVertices |
        aiProcess_CalcTangentSpace;

    //// aiProcess_RemoveComponent |
    //// aiProcess_SplitLargeMeshes |
    //// aiProcess_ValidateDataStructure |
    //////aiProcess_ImproveCacheLocality | // handled by optimizePostTransform()
    //// aiProcess_RemoveRedundantMaterials |
    // aiProcess_SortByPType |
    //// aiProcess_FindInvalidData |
    //// aiProcess_GenUVCoords |
    //// aiProcess_TransformUVCoords |
    //// aiProcess_OptimizeMeshes |
    //// aiProcess_OptimizeGraph;

    //// aiProcess_RemoveComponent |
    // aiProcess_GenBoundingBoxes |
    //////aiProcess_SplitLargeMeshes |
    //////aiProcess_ValidateDataStructure |
    // aiProcess_FlipUVs | aiProcess_MakeLeftHanded |
    //// aiProcess_ConvertToLeftHanded |
    // aiProcess_ImproveCacheLocality | aiProcess_FlipWindingOrder |
    //// aiProcess_RemoveRedundantMaterials | // remove redundant materials
    //// aiProcess_FindDegenerates | // remove degenerated polygons from the import
    //// aiProcess_FindInvalidData | // detect invalid model data, such as invalid normal vectors
    //// aiProcess_GenUVCoords | // convert spherical, cylindrical, box and planar mapping to proper UVs
    // aiProcess_TransformUVCoords | // preprocess UV transformations (scaling, translation ...)
    //// aiProcess_OptimizeMeshes | // join small meshes, if possible;
    // aiProcess_PreTransformVertices | //-- fixes the transformation issue.
    // 0;

    const std::u8string pu8 = absolutePath.u8string();
    const std::string pathUtf8(reinterpret_cast<const char*>(pu8.data()), pu8.size());
    const aiScene* scene = importer.ReadFile(pathUtf8.c_str(), kImportFlags);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        DLOG(LogAsset, ELogLevel::Error,
            "DMesh ImportMeshImpl: Assimp load failed path='{}' sceneValid={} incompleteFlag={:#x} rootNodeValid={} error='{}'",
            pathUtf8,
            static_cast<bool>(scene),
            scene ? scene->mFlags : 0u,
            static_cast<bool>(scene && scene->mRootNode),
            importer.GetErrorString());
        return;
    }

    ProcessNode(scene->mRootNode, scene, DirectX::XMMatrixIdentity(), absolutePath, loadTextures);
}

void DMesh::ImportMesh()
{
    ImportMeshImpl(IOManager::GetEngineSourceAssetFullPath(m_sourcePath), true);
}

void DMesh::ProcessNode(aiNode* node, const aiScene* scene, DirectX::XMMATRIX accTransform, const std::filesystem::path& absolutePath, bool loadTextures)
{
    if (!node || !scene)
    {
        DLOG(LogAsset, ELogLevel::Warning,
            "ProcessNode: null node or scene (node={}, scene={}) — skipping subtree",
            static_cast<const void*>(node), static_cast<const void*>(scene));
        return;
    }

    const auto localTransformation = DirectX::XMMATRIX(&(node->mTransformation.a1));
    accTransform = DirectX::XMMatrixMultiply(accTransform, localTransformation);

    for (unsigned int meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex)
        ProcessMesh(scene->mMeshes[node->mMeshes[meshIndex]], scene, absolutePath, loadTextures);

    for (unsigned int childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
        ProcessNode(node->mChildren[childIndex], scene, accTransform, absolutePath, loadTextures);
}

void DMesh::ProcessMesh(aiMesh* mesh, const aiScene* scene, const std::filesystem::path& absolutePath, bool loadTextures)
{
    if (!mesh || !scene)
    {
        DLOG(LogAsset, ELogLevel::Warning,
            "ProcessMesh: null mesh or scene (mesh={}, scene={})",
            static_cast<const void*>(mesh), static_cast<const void*>(scene));
        return;
    }

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<DTexture*> textures;

    for (unsigned int vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex)
    {
        Vertex vertex {};
        vertex.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        vertex.position = DirectX::XMFLOAT3(&mesh->mVertices[vertexIndex].x);
        if (mesh->HasNormals())
            vertex.normal = DirectX::XMFLOAT3(&mesh->mNormals[vertexIndex].x);
        else
        {
            vertex.normal = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
            if (vertexIndex == 0)
                DLOG(LogAsset, ELogLevel::Warning,
                    "ProcessMesh: mesh '{}' missing normals — using fallback up-vector",
                    mesh->mName.C_Str());
        }

        if (mesh->HasTangentsAndBitangents())
            vertex.tangent = DirectX::XMFLOAT3(&mesh->mTangents[vertexIndex].x);

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

    if (loadTextures && mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex < scene->mNumMaterials)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::vector<DTexture*> diffuseMaps = LoadMaterialTextures(scene, material, aiTextureType_DIFFUSE, "texture_diffuse", absolutePath);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        std::vector<DTexture*> specularMaps = LoadMaterialTextures(scene, material, aiTextureType_SPECULAR, "texture_specular", absolutePath);
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    }
    else if (loadTextures && mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex >= scene->mNumMaterials)
    {
        DLOG(LogAsset, ELogLevel::Warning,
            "ProcessMesh: material index {} out of range (numMaterials={}) mesh='{}'",
            mesh->mMaterialIndex,
            scene->mNumMaterials,
            mesh->mName.C_Str());
    }

    m_vertices.push_back(std::move(vertices));
    m_indices.push_back(std::move(indices));
    m_textures.insert(m_textures.end(), textures.begin(), textures.end());
}

std::vector<DTexture*> DMesh::LoadMaterialTextures(const aiScene* scene, aiMaterial* mat, aiTextureType type, std::string typeName, const std::filesystem::path& filePath)
{
    (void)scene;

    std::vector<DTexture*> textures;
    if (!mat)
    {
        DLOG(LogAsset, ELogLevel::Warning,
            "LoadMaterialTextures: null aiMaterial type={} tag='{}'",
            static_cast<int>(type), typeName);
        return textures;
    }

    const std::filesystem::path folderPath = ParentPathLevelsUp(filePath, 2);
    for (unsigned int textureIndex = 0; textureIndex < mat->GetTextureCount(type); ++textureIndex)
    {
        aiString str;
        mat->GetTexture(type, textureIndex, &str);

        const std::string importedPath = str.C_Str();
        if (importedPath.empty())
        {
            DLOG(LogAsset, ELogLevel::Verbose,
                "LoadMaterialTextures: empty imported texture path slot={} type={} tag='{}'",
                textureIndex, static_cast<int>(type), typeName);
            continue;
        }

        const auto fileName = importedPath.substr(importedPath.find_last_of("\\/") + 1);
        const std::filesystem::path texturePath = FindTextureFile(folderPath, fileName);
        if (texturePath.empty() || !std::filesystem::exists(texturePath))
        {
            DLOG(LogAsset, ELogLevel::Warning,
                "LoadMaterialTextures: texture '{}' not resolved under '{}' (meshFolder={}, type={}, tag='{}')",
                fileName, PathLog(folderPath), PathLog(filePath), static_cast<int>(type), typeName);
            continue;
        }

        try
        {
            DTexture* texture = DTexture::LoadFromFile(texturePath, true);
            textures.push_back(texture);
        }
        catch (const std::exception& ex)
        {
            DLOG(LogAsset, ELogLevel::Warning,
                "LoadMaterialTextures: exception loading '{}' — {}",
                PathLog(texturePath), ex.what());
        }
    }

    return textures;
}

void DMesh::Initialize(const std::filesystem::path& sourcePath)
{
    m_sourcePath = sourcePath;
    ImportMesh();
}

void DMesh::ImportFromAbsolutePath(const std::filesystem::path& absolutePath)
{
    m_sourcePath = absolutePath;
    ImportMeshImpl(m_sourcePath, false);
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
