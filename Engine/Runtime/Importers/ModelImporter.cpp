#include "ModelImporter.h"

#include <DirectXMath.h>
#include <filesystem>
#include <iostream>

#include "Graphics/Structures/Vertex.h"
#include "Graphics/Mesh.h"
#include "IO/IOManager.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

using namespace DeltaEngine;
using namespace std;

ModelImporter::ModelImporter() {}

ModelImporter::~ModelImporter() {}

void ModelImporter::Import(const std::string& filePath)
{
	this->sourcePath = filePath;
    //Assimp::Importer::SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, true)
	auto fullPath = IOManager::GetAssetFullPath(filePath);
	Assimp::Importer import;
    import.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, true);
	unsigned int flags = 
        //aiProcess_CalcTangentSpace |
        //aiProcess_JoinIdenticalVertices |
        aiProcess_Triangulate |
        //aiProcess_RemoveComponent |
        //aiProcess_GenSmoothNormals |
        //aiProcess_SplitLargeMeshes |
        //aiProcess_ValidateDataStructure |
        aiProcess_FlipUVs | 
        aiProcess_MakeLeftHanded | 
        aiProcess_FlipWindingOrder | 
        aiProcess_RemoveRedundantMaterials | // remove redundant materials
        aiProcess_FindDegenerates | // remove degenerated polygons from the import
        aiProcess_FindInvalidData | // detect invalid model data, such as invalid normal vectors
        aiProcess_GenUVCoords | // convert spherical, cylindrical, box and planar mapping to proper UVs
        aiProcess_TransformUVCoords | // preprocess UV transformations (scaling, translation ...)
        aiProcess_OptimizeMeshes | // join small meshes, if possible;
        aiProcess_PreTransformVertices //-- fixes the transformation issue.
        ;
    const aiScene *scene = import.ReadFile(fullPath, flags);
	
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
    {
        // cout << "ERROR::ASSIMP::" << import.GetErrorString() << endl;
        return;
    }
    // directory = filePath.substr(0, filePath.find_last_of('/'));

	ProcessNode(scene->mRootNode, scene, DirectX::XMMatrixIdentity());
}

void ModelImporter::ProcessNode(aiNode *node, const aiScene *scene, DirectX::XMMATRIX accTransform)
{
    aiVector3D scaling, position;
    aiQuaternion rotation;
    node->mTransformation.Decompose(scaling, rotation, position);
    DxTransform dxTransform {
        DirectX::XMVectorSet(position.x, position.y, position.z, 1.0f), 
        DirectX::XMVectorSet(rotation.x, rotation.y, rotation.z, rotation.w), 
        DirectX::XMVectorSet(scaling.x, scaling.y, scaling.z, 1.0f) 
    };

	auto dxmTrans = DirectX::XMMatrixTransformation(
		DirectX::XMVectorZero(), 
        DirectX::XMQuaternionIdentity(),
		dxTransform.scale,

		DirectX::XMVectorZero(), 
        dxTransform.rotation,

		dxTransform.position
	);

    accTransform = XMMatrixMultiply(accTransform, dxmTrans);

    //node->mTransformation
    // process all the node's meshes (if any)
 //   auto nodeTransform = node->mTransformation;
	//auto nodeLookup = node;
 //   while (nodeLookup->mParent != nullptr) {
	//	nodeLookup = nodeLookup->mParent;
	//	nodeTransform = nodeLookup->mTransformation * nodeTransform;
 //   }

	//auto nodeTransform2 = DirectX::XMMATRIX(&nodeTransform.a1);

    //auto transform = DirectX::XMMATRIX(&node->mTransformation.a1);
    //accTransform = XMMatrixMultiply(transform, accTransform);
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
        meshes.push_back(ProcessMesh(mesh, scene));	
		meshTransforms.push_back(accTransform);
		//meshDxTransforms.push_back(dxTransform);
    }
    // then do the same for each of its children
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene, accTransform);
    }
}

Mesh *ModelImporter::ProcessMesh(aiMesh *mesh, const aiScene *scene)
{
	std::vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture*> textures;
    for(unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
		vertex.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        // process vertex positions, normals and texture coordinates
        DirectX::XMFLOAT3 position; 
        position.x = mesh->mVertices[i].x;
		position.y = mesh->mVertices[i].y;
		position.z = mesh->mVertices[i].z;
		vertex.position = position;
        vertices.push_back(vertex);

		DirectX::XMFLOAT3 vector;
        vector.x = mesh->mNormals[i].x;
		vector.y = mesh->mNormals[i].y;
		vector.z = mesh->mNormals[i].z;
		vertex.normal = vector;

        if(mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
		{
		    DirectX::XMFLOAT2 vec;
		    vec.x = mesh->mTextureCoords[0][i].x; 
		    vec.y = mesh->mTextureCoords[0][i].y;
		    vertex.uv = vec;
		}
		else
		    vertex.uv = DirectX::XMFLOAT2(0.0f, 0.0f);
    }
    // process indices
    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
	    aiFace face = mesh->mFaces[i];
	    for(unsigned int j = 0; j < face.mNumIndices; j++)
	        indices.push_back(face.mIndices[j]);
	}

    // process material
    if(mesh->mMaterialIndex >= 0)
    {
        if(mesh->mMaterialIndex >= 0)
		{
		    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
		    vector<Texture*> diffuseMaps = LoadMaterialTextures(scene, material, 
		                                        aiTextureType_DIFFUSE, "texture_diffuse", this->sourcePath);
		    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

		    vector<Texture*> specularMaps = LoadMaterialTextures(scene, material, 
		                                        aiTextureType_SPECULAR, "texture_specular", this->sourcePath);
		    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		}  
    }

    return new Mesh(vertices, indices, textures);
}

std::string GetParentDirectory(const std::string& filePath, int levelsUp) {
    namespace fs = std::filesystem;
    fs::path path(filePath);
    for (int i = 0; i < levelsUp; ++i) {
        path = path.parent_path();
    }
    return path.string();
}

std::string FindTextureFile(const std::string& directory, const std::string& fileName) {
    namespace fs = std::filesystem;
    auto fullDirectory = IOManager::GetAssetFullPath(directory);
    for (const auto& entry : fs::recursive_directory_iterator(fullDirectory)) {
        if (entry.is_regular_file() && entry.path().filename() == fileName) {
            return entry.path().string();
        }
    }
    return "";
}

vector<Texture*> ModelImporter::LoadMaterialTextures(const aiScene* scene, aiMaterial *mat, aiTextureType type, string typeName, const std::string& filePath)
{
	auto folderPath = GetParentDirectory(filePath, 2);
    vector<Texture*> textures;
    for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
		//auto t = scene->GetEmbeddedTexture(str.C_Str());
        std::string filePath = str.C_Str();
        auto fileName = filePath.substr(filePath.find_last_of("\\/") + 1);
		//auto fullFolderPath = IOManager::GetAssetFullPath(folderPath);
		auto texturePath = FindTextureFile(folderPath, fileName);
        auto texture = Texture::LoadFromFile(texturePath, true);
         //texture.id = TextureFromFile(str.C_Str(), directory);
         //texture.type = typeName;
         //texture.path = str;
        textures.push_back(texture);
    }
    return textures;
}
