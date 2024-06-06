#pragma once
#include <vector>
#include <string>

class Texture
{
public:
	Texture(const std::string &filePath);
	~Texture();

	unsigned int GetWidth() const { return width; }
	unsigned int GetHeight() const { return height; }
	const std::vector<unsigned char> &GetData() const { return data; }

	static Texture* LoadFromFile(const std::string& filePath);

private:
	unsigned int width;
	unsigned int height;
	std::vector<unsigned char> data;
};
