#include "Texture.h"

#include "IO/lodepng.h"
#include "IO/IOManager.h"

using namespace DeltaEngine;

Texture::Texture(const std::string &filePath)
{
    auto fullPath = IOManager::GetAssetFullPath(filePath);
    lodepng::decode(data, width, height, fullPath, LCT_RGBA, 8);
}

Texture::~Texture()
{
}

Texture* Texture::LoadFromFile(const std::string& filePath) {
    return new Texture(filePath);
}
// data = { size=319872 }