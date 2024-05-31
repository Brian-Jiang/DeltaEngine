#pragma once

#include <string>

#include "GLTexture.h"

class ImageLoader
{
public:
	static GLTexture loadPNGForGL(std::string filePath);
};

