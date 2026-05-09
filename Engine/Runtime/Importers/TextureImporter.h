#pragma once

#include "EngineIncludes.h"

#include <filesystem>
#include <vector>
#include "Importers/BaseImporter.h"

DELTA_ENGINE_NS_BEGIN

class TextureImporter : public BaseImporter
{
public:
	void Import(const std::filesystem::path& filePath) override;

	std::vector<unsigned char> data;
	int width;
	int height;
	int comp;
};

DELTA_ENGINE_NS_END