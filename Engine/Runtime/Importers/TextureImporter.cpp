#include "Importers/TextureImporter.h"

#define STB_IMAGE_IMPLEMENTATION
#include "IO/stb_image.h"

#include <iostream>

using namespace DeltaEngine;

void TextureImporter::Import(const std::string& filePath) {
	//stbi_set_flip_vertically_on_load(true);
	unsigned char* d = stbi_load(filePath.c_str(), &width, &height, &comp, 0);
	data = std::vector<unsigned char>(d, d + width * height * comp);

	//std::cout << "Width: " << width << std::endl;
	//std::cout << "Height: " << height << std::endl;
	//std::cout << "comp: " << comp << std::endl;
	//for (int i = 0; i < std::min(width * height * 4, 1024000); i += 4) {
	//	std::cout << "Pixel " << i / 4 << ": " << (int)d[i] << " " << (int)d[i + 1] << " " << (int)d[i + 2] << " " << (int)d[i + 3] << std::endl;
	//}
}
