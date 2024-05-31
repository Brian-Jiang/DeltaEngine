#pragma once
#include "GLSLProgram.h"
#include "GLTexture.h"

class SpriteRenderer
{
public:
	SpriteRenderer();
	~SpriteRenderer();

	void Start(float x, float y, float width, float height, const char* texturePath, GLSLProgram *program);
	void Render();

private:
	float x;
	float y;
	float width;
	float height;
	// const char* texturePath;
	// unsigned int texture;
	unsigned int vbo;
	unsigned int VAO;
	unsigned int EBO;
	GLTexture texture;
	GLSLProgram* program;
};

