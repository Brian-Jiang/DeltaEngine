#pragma once

#include "../../ThirdParty/glew/include/GL/glew.h"

class SpriteRenderer
{
public:
	SpriteRenderer();
	~SpriteRenderer();

	void Start(float x, float y, float width, float height, const char* texturePath);
	void Render();

private:
	float x;
	float y;
	float width;
	float height;
	const char* texturePath;
	unsigned int texture;
	unsigned int vbo;
	unsigned int VAO;
	unsigned int EBO;
};

