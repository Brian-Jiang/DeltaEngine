#include "Graphics/Mesh.h"

using namespace DeltaEngine;

Mesh::Mesh() {}

Mesh::Mesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices,
    std::vector<std::shared_ptr<DTexture>>& textures)
{
    this->vertices = vertices;
	this->indices = indices;
	this->textures = textures;
}

