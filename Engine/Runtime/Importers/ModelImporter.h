#pragma once

#include "EngineIncludes.h"

#include <string>
#include <vector>

#include "assimp/scene.h"
#include "Graphics/Mesh.h"
#include "Importers/BaseImporter.h"
#include "Graphics/Texture.h"

DELTA_ENGINE_NS_BEGIN

class ModelImporter : public BaseImporter
{
public:
	std::vector<Mesh*> meshes;
	std::vector<Texture*> textures;

	ModelImporter();
	~ModelImporter();

	void Import(const std::string& filePath) override;
	void ProcessNode(aiNode* node, const aiScene* scene);
	Mesh *ProcessMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture*> LoadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
};

DELTA_ENGINE_NS_END
