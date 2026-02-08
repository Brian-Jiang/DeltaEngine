#pragma once

#include "EngineIncludes.h"

#include <string>
#include <vector>

#include "assimp/scene.h"
#include "Graphics/Mesh.h"
#include "Importers/BaseImporter.h"
#include "Graphics/DTexture.h"

DELTA_ENGINE_NS_BEGIN

struct DxTransform {
	DirectX::XMVECTOR position;
	DirectX::XMVECTOR rotation;
	DirectX::XMVECTOR scale;
};

class ModelImporter : public BaseImporter
{
public:
	std::vector<Mesh*> meshes;
	std::vector<DirectX::XMMATRIX> meshTransforms;
	std::vector<DxTransform> meshDxTransforms;
	std::vector<std::shared_ptr<DTexture>> textures;

	ModelImporter();
	~ModelImporter();

	void Import(const std::string& filePath) override;
	void ProcessNode(aiNode* node, const aiScene* scene, DirectX::XMMATRIX accTransform);
	Mesh *ProcessMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<std::shared_ptr<DTexture>> LoadMaterialTextures(const aiScene* scene, aiMaterial* mat, aiTextureType type, std::string typeName, const std::string& filePath);

	std::string sourcePath;
};

DELTA_ENGINE_NS_END
