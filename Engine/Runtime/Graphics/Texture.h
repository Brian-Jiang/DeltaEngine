//#pragma once
//
//#include <vector>
//#include <string>
//#include <d3d12.h>
//
//namespace DeltaEngine
//{
//
//class Texture
//{
//public:
//	Texture(const std::string &filePath, bool isFullPath);
//	~Texture();
//
//	unsigned int GetWidth() const { return width; }
//	unsigned int GetHeight() const { return height; }
//	const std::vector<unsigned char> &GetData() const { return data; }
//
//	static Texture* LoadFromFile(const std::string& filePath, bool isFullPath = false);
//
//	int pixelSize;
//	DXGI_FORMAT format;
//	std::string name;
//
//private:
//	unsigned int width;
//	unsigned int height;
//	std::vector<unsigned char> data;
//};
//
//}
