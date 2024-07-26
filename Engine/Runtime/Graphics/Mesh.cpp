#include "Graphics/Mesh.h"

DeltaEngine::Mesh::Mesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices,
    std::vector<Texture*>& textures)
{
    this->vertices = vertices;
}

