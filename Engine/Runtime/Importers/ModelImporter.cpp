#include "ModelImporter.h"

#include <DirectXMath.h>
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
	auto fullPath = IOManager::GetAssetFullPath(filePath);
	Assimp::Importer import;
    const aiScene *scene = import.ReadFile(fullPath, aiProcess_Triangulate | aiProcess_FlipUVs);
	
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
    {
        // cout << "ERROR::ASSIMP::" << import.GetErrorString() << endl;
        return;
    }
    // directory = filePath.substr(0, filePath.find_last_of('/'));

    ProcessNode(scene->mRootNode, scene);
}

void ModelImporter::ProcessNode(aiNode *node, const aiScene *scene)
{
    // process all the node's meshes (if any)
    for(unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; 
        meshes.push_back(ProcessMesh(mesh, scene));			
    }
    // then do the same for each of its children
    for(unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene);
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
        // process vertex positions, normals and texture coordinates
        DirectX::XMFLOAT3 vector; 
		vector.x = mesh->mVertices[i].x;
		vector.y = mesh->mVertices[i].y;
		vector.z = mesh->mVertices[i].z; 
		vertex.position = vector;
        vertices.push_back(vertex);

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
		    vector<Texture*> diffuseMaps = LoadMaterialTextures(material, 
		                                        aiTextureType_DIFFUSE, "texture_diffuse");
		    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		    vector<Texture*> specularMaps = LoadMaterialTextures(material, 
		                                        aiTextureType_SPECULAR, "texture_specular");
		    textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		}  
    }

    return new Mesh(vertices, indices, textures);
}

vector<Texture*> ModelImporter::LoadMaterialTextures(aiMaterial *mat, aiTextureType type, string typeName)
{
    vector<Texture*> textures;
    for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        auto texture = Texture::LoadFromFile(str.C_Str());
        // texture.id = TextureFromFile(str.C_Str(), directory);
        // texture.type = typeName;
        // texture.path = str;
        textures.push_back(texture);
    }
    return textures;
}
