#include "IOManager.h"

#include <fstream>
#include <vector>

bool IOManager::readFileToBuffer(const std::string& filePath, std::vector<char>& buffer) {
	std::ifstream file(filePath, std::ios::binary);
	if (file.fail()) {
		perror(filePath.c_str());
		return false;
	}

	// Seek to the end
	file.seekg(0, std::ios::end);
	 
	// Get the file size
	auto fileSize = file.tellg();
	file.seekg(0, std::ios::beg);
 
	// Reduce the file size by any header bytes that might be present
	fileSize -= file.tellg();
 
	buffer.resize(fileSize);
	file.read(&(buffer[0]), fileSize);
	file.close();
 
	return true;
 }
