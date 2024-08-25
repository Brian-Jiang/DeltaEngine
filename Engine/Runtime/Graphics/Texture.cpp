#include "Texture.h"

#include "IO/lodepng.h"
#include "IO/IOManager.h"
#include "Importers/TextureImporter.h"

using namespace DeltaEngine;

Texture::Texture(const std::string &filePath)
{
    auto fullPath = IOManager::GetAssetFullPath(filePath);
	auto importer = new TextureImporter();
	importer->Import(fullPath);
	width = importer->width;
	height = importer->height;
	data = importer->data;
    //lodepng::decode(data, width, height, fullPath, LCT_RGBA, 8);
}

Texture::~Texture()
{
}

Texture* Texture::LoadFromFile(const std::string& filePath) {
    return new Texture(filePath);
}
// data = { size=319872 }