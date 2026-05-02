#include "Editor/Assets/AssetImporter.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Runtime/Assets/PA_CommonAssets.h"
#include "Runtime/Core/DMaterial.h"
#include "Runtime/Core/DMesh.h"
#include "Runtime/Core/DShader.h"
#include "Runtime/Core/DTexture.h"
#include "Runtime/IO/IOManager.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

using namespace DeltaEngine;

namespace
{

std::string FindTextureFile(const std::string& directory, const std::string& fileName)
{
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
    {
        if (entry.is_regular_file() && entry.path().filename() == fileName)
            return entry.path().string();
    }
    return {};
}

std::string GetSearchDirectory(const std::filesystem::path& fbxPath)
{
    // Search 2 levels above the FBX (same convention as DMesh::LoadMaterialTextures)
    std::filesystem::path dir = fbxPath.parent_path().parent_path();
    return dir.string();
}

} // namespace

std::filesystem::path AssetImporter::ResolveDestPath(const std::filesystem::path& dir, const std::string& baseName, EditorAssetDatabase& db)
{
    std::filesystem::path candidate = dir / (baseName + ".dasset.json");
    if (!std::filesystem::exists(candidate) && db.FindAssetIdByPath(candidate).IsNull())
        return candidate;

    for (int suffix = 1; suffix < 10000; ++suffix)
    {
        candidate = dir / (baseName + "_" + std::to_string(suffix) + ".dasset.json");
        if (!std::filesystem::exists(candidate) && db.FindAssetIdByPath(candidate).IsNull())
            return candidate;
    }

    return candidate;
}

AssetId AssetImporter::ImportTexture(const std::filesystem::path& sourcePath, const std::filesystem::path& destDir, EditorAssetDatabase& db)
{
    DTexture* texture = CreateDObject<DTexture>();
    texture->Initialize(sourcePath);

    PA_Texture* pa = PA_Texture::Create(texture);
    const std::filesystem::path dest = ResolveDestPath(destDir, sourcePath.stem().string(), db);
    db.CreateAsset(dest, pa);
    return pa->GetAssetId();
}

AssetId AssetImporter::ImportShader(const std::filesystem::path& sourcePath, const std::filesystem::path& destDir, EditorAssetDatabase& db)
{
    DShader* shader = CreateDObject<DShader>();
    shader->Initialize(sourcePath, "VSMain", "PSMain", "vs_6_6", "ps_6_6");

    PA_Shader* pa = PA_Shader::Create(shader);
    const std::filesystem::path dest = ResolveDestPath(destDir, sourcePath.stem().string(), db);
    db.CreateAsset(dest, pa);
    return pa->GetAssetId();
}

std::vector<AssetId> AssetImporter::ImportFbx(const std::filesystem::path& sourcePath, EditorAssetDatabase& db)
{
    std::vector<AssetId> createdIds;

    const std::filesystem::path importedRoot(IOManager::GetEngineImportedAssetsFolder());
    const std::filesystem::path subDir = importedRoot / sourcePath.stem().string();
    std::filesystem::create_directories(subDir);

    // -----------------------------------------------------------------------
    // Pass 1: parse FBX to extract texture and material info
    // -----------------------------------------------------------------------
    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, true);

    constexpr unsigned int kFlags =
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_FlipUVs |
        aiProcess_MakeLeftHanded |
        aiProcess_FlipWindingOrder |
        aiProcess_PreTransformVertices |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType;

    const aiScene* scene = importer.ReadFile(sourcePath.string(), kFlags);
    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
        return createdIds;

    const std::string searchDir = GetSearchDirectory(sourcePath);

    // -----------------------------------------------------------------------
    // Pass 2: create PA_Texture assets for each unique texture file
    // -----------------------------------------------------------------------
    // Maps aiTexture path string → DTexture* (inside a PA_Texture)
    std::unordered_map<std::string, DTexture*> textureMap;

    constexpr aiTextureType kTextureSlots[] = {
        aiTextureType_DIFFUSE,
        aiTextureType_NORMALS,
        aiTextureType_METALNESS,
        aiTextureType_DIFFUSE_ROUGHNESS,
        aiTextureType_EMISSIVE,
        aiTextureType_AMBIENT_OCCLUSION,
    };

    for (unsigned int matIdx = 0; matIdx < scene->mNumMaterials; ++matIdx)
    {
        const aiMaterial* mat = scene->mMaterials[matIdx];
        for (const aiTextureType slot : kTextureSlots)
        {
            for (unsigned int texIdx = 0; texIdx < mat->GetTextureCount(slot); ++texIdx)
            {
                aiString aiPath;
                mat->GetTexture(slot, texIdx, &aiPath);
                const std::string texPath = aiPath.C_Str();
                if (texPath.empty() || textureMap.count(texPath))
                    continue;

                const std::string fileName = std::filesystem::path(texPath).filename().string();
                const std::string fullTexPath = FindTextureFile(searchDir, fileName);
                if (!std::filesystem::exists(fullTexPath))
                    continue;

                const std::filesystem::path texSrc(fullTexPath);
                DTexture* texture = CreateDObject<DTexture>();
                texture->Initialize(texSrc);

                PA_Texture* pa = PA_Texture::Create(texture);
                const std::filesystem::path dest = ResolveDestPath(subDir, texSrc.stem().string(), db);
                db.CreateAsset(dest, pa);
                createdIds.push_back(pa->GetAssetId());

                textureMap[texPath] = texture;
            }
        }
    }

    // -----------------------------------------------------------------------
    // Pass 3: create PA_Material assets
    // -----------------------------------------------------------------------
    // Indexed by aiMesh::mMaterialIndex
    std::vector<DMaterial*> importedMaterials(scene->mNumMaterials, nullptr);

    for (unsigned int matIdx = 0; matIdx < scene->mNumMaterials; ++matIdx)
    {
        const aiMaterial* aiMat = scene->mMaterials[matIdx];

        aiString matName;
        aiMat->Get(AI_MATKEY_NAME, matName);
        std::string name = matName.C_Str();
        if (name.empty())
            name = "Material_" + std::to_string(matIdx);

        DMaterial* mat = CreateDObject<DMaterial>();
        mat->Initialize(nullptr);

        auto assignTexture = [&](aiTextureType slot, void (DMaterial::*setter)(DTexture*))
        {
            if (aiMat->GetTextureCount(slot) > 0)
            {
                aiString aiPath;
                aiMat->GetTexture(slot, 0, &aiPath);
                auto it = textureMap.find(aiPath.C_Str());
                if (it != textureMap.end())
                    (mat->*setter)(it->second);
            }
        };

        assignTexture(aiTextureType_DIFFUSE,           &DMaterial::SetAlbedoTexture);
        assignTexture(aiTextureType_NORMALS,           &DMaterial::SetNormalTexture);
        assignTexture(aiTextureType_METALNESS,         &DMaterial::SetMetallicRoughnessTexture);
        assignTexture(aiTextureType_AMBIENT_OCCLUSION, &DMaterial::SetOcclusionTexture);
        assignTexture(aiTextureType_EMISSIVE,          &DMaterial::SetEmissiveMaskTexture);

        PA_Material* pa = PA_Material::Create(mat);
        const std::filesystem::path dest = ResolveDestPath(subDir, name, db);
        db.CreateAsset(dest, pa);
        createdIds.push_back(pa->GetAssetId());

        importedMaterials[matIdx] = mat;
    }

    // -----------------------------------------------------------------------
    // Pass 4: create PA_StaticMesh (geometry only, materials wired up)
    // -----------------------------------------------------------------------
    DMesh* mesh = CreateDObject<DMesh>();
    mesh->ImportFromAbsolutePath(sourcePath);
    mesh->SetMaterials(importedMaterials);

    PA_StaticMesh* pa = PA_StaticMesh::Create(mesh);
    const std::filesystem::path meshDest = ResolveDestPath(subDir, sourcePath.stem().string(), db);
    db.CreateAsset(meshDest, pa);
    createdIds.push_back(pa->GetAssetId());

    return createdIds;
}

std::vector<AssetId> AssetImporter::ImportFile(const std::filesystem::path& sourcePath, EditorAssetDatabase& db)
{
    const std::string ext = sourcePath.extension().string();
    const std::filesystem::path importedRoot(IOManager::GetEngineImportedAssetsFolder());
    std::filesystem::create_directories(importedRoot);

    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" ||
        ext == ".tga" || ext == ".dds" || ext == ".hdr")
    {
        const AssetId id = ImportTexture(sourcePath, importedRoot, db);
        if (!id.IsNull())
            return { id };
        return {};
    }

    if (ext == ".fbx" || ext == ".obj")
        return ImportFbx(sourcePath, db);

    if (ext == ".slang")
    {
        const AssetId id = ImportShader(sourcePath, importedRoot, db);
        if (!id.IsNull())
            return { id };
        return {};
    }

    return {};
}
