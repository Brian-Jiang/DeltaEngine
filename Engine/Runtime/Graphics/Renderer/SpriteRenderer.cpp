#include "SpriteRenderer.h"

#include <cstddef>

#include "ImageLoader.h"
#include "GL/glew.h"
#include "Vertex.h"


SpriteRenderer::SpriteRenderer()
{
}

SpriteRenderer::~SpriteRenderer()
{
	if (this->vbo != 0)
	{
		glDeleteBuffers(1, &this->vbo);
	}
}

void SpriteRenderer::Start(float x, float y, float width, float height, const char* texturePath, GLSLProgram *program)
{
	this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;

	if (this->vbo == 0)
	{
		glGenBuffers(1, &this->vbo);
	}

	Vertex vertexData[6];
	vertexData[0].position.x = x;
	vertexData[0].position.y = y + height;
	vertexData[0].SetUV(0.0f, 1.0f);

	vertexData[1].position.x = x;
	vertexData[1].position.y = y;
	vertexData[1].SetUV(0.0f, 0.0f);

	vertexData[2].position.x = x + width;
	vertexData[2].position.y = y;
	vertexData[2].SetUV(1.0f, 0.0f);

	vertexData[3].position.x = x;
	vertexData[3].position.y = y + height;
	vertexData[3].SetUV(0.0f, 1.0f);

	vertexData[4].position.x = x + width;
	vertexData[4].position.y = y + height;
	vertexData[4].SetUV(1.0f, 1.0f);

	vertexData[5].position.x = x + width;
	vertexData[5].position.y = y;
	vertexData[5].SetUV(1.0f, 0.0f);

	// for (int i = 0; i < 6; i++) {
	// 	vertexData[i].color.r = 255;
	// 	vertexData[i].color.g = 255;
	// 	vertexData[i].color.b = 255;
	// 	vertexData[i].color.a = 255;
	// }
	//
	// vertexData[0].color.r = 0;
	// vertexData[1].color.g = 0;
	// vertexData[2].color.b = 0;
	// vertexData[3].color.b = 0;
	// vertexData[3].color.r = 128;

	// glGenVertexArrays(1, &this->VAO);
	// glBindVertexArray(this->VAO);

	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	// glEnableVertexAttribArray(0);
	//
	// if (this->EBO == 0)
	// {
	// 	glGenBuffers(1, &this->EBO);
	// }
	//
	// unsigned int indices[6] = { 0, 1, 3, 1, 2, 3 };
	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
	// glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	//
	this->texture = ImageLoader::loadPNGForGL(texturePath);
	this->program = program;
}

void SpriteRenderer::Render()
{
	glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture.textureID);

	auto textureLocation = program->getUniformLocation("sampler");
    glUniform1i(textureLocation, 0);

	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, position));
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*) offsetof(Vertex, color));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, uv));
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}
