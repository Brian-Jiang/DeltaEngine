#include "ImageLoader.h"

#include "lodepng.h"


// #include "sail-c++/image.h"


GLTexture ImageLoader::loadPNG(std::string filePath)
{
	GLTexture glTexture = {};
	// sail::image image(filePath);
	// glTexture.height = image.height();
	// glTexture.width = image.width();

	std::vector<unsigned char> image;
	unsigned int width, height;
	lodepng::decode(image, width, height, filePath, LCT_RGBA, 8);
	glTexture.height = height;
	glTexture.width = width;

	glGenTextures(1, &glTexture.textureID);
	glBindTexture(GL_TEXTURE_2D, glTexture.textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glTexture.width, glTexture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &(image[0]));

	glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);

	return glTexture;
}
