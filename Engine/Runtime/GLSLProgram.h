#pragma once
#include <string>

#include "GL/glew.h"

class GLSLProgram
{
public:
	GLSLProgram();
	~GLSLProgram();
	void compileShaders(const std::string& vertexShaderFilePath, const std::string& fragmentShaderFilePath);
	// void linkShaders();
	// void addAttribute(const std::string& attributeName);
	// GLint getUniformLocation(const std::string& uniformName);
	GLint getUniformLocation(const std::string& uniformName);
	void use();
	void unuse();

private:
	// int _numAttributes;
	// void compileShader(const std::string& filePath, GLuint id);
	GLuint programID;
	GLuint vertexShaderID;
	GLuint fragmentShaderID;
};

