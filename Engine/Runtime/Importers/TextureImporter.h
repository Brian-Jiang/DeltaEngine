#pragma once

#include "EngineIncludes.h"

#include <vector>
#include "Importers/BaseImporter.h"

DELTA_ENGINE_NS_BEGIN

class TextureImporter : public BaseImporter
{
public:
	void Import(const std::string& filePath) override;

	std::vector<unsigned char> data;
	int width;
	int height;

};

DELTA_ENGINE_NS_END