#pragma once

#include "EngineIncludes.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

class BaseImporter
{
public:
	BaseImporter();
	~BaseImporter();

	virtual void Import(const std::wstring& filePath) = 0;
};

DELTA_ENGINE_NS_END
