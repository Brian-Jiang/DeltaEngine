#pragma once

#include "EngineIncludes.h"

#include <filesystem>

DELTA_ENGINE_NS_BEGIN

class BaseImporter
{
public:
	BaseImporter();
	~BaseImporter();

	virtual void Import(const std::filesystem::path& filePath) = 0;
};

DELTA_ENGINE_NS_END
