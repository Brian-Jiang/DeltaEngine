#include "Texture.h"

#include "IO/lodepng.h"
#include "IO/IOManager.h"
#include "Importers/TextureImporter.h"

using namespace DeltaEngine;

Texture::Texture(const std::string &filePath, bool isFullPath)
{
	this->name = filePath;
	std::string fullPath = filePath;
	if (!isFullPath) {
		fullPath = IOManager::GetAssetFullPath(filePath);
	}
	auto importer = new TextureImporter();
	importer->Import(fullPath);
	width = importer->width;
	height = importer->height;
	data = importer->data;
	pixelSize = importer->comp;
    //lodepng::decode(data, width, height, fullPath, LCT_RGBA, 8);

	if (pixelSize == 4) {
		this->format = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	else if (pixelSize == 3) {
		this->format = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	else {
		this->format = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
}

Texture::~Texture()
{
}

Texture* Texture::LoadFromFile(const std::string& filePath, bool isFullPath) {
    return new Texture(filePath, isFullPath);
}
// data = { size=319872 }