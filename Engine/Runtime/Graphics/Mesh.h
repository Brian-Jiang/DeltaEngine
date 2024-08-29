#pragma once

#include "EngineIncludes.h"

#include <vector>

#include "Core/Object.h"
#include "Graphics/Structures/Vertex.h"
#include "Graphics/Texture.h"

DELTA_ENGINE_NS_BEGIN

class Mesh : public Object
{
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture*> textures;

    Mesh();
    Mesh(std::vector<Vertex> &vertices, std::vector<unsigned int> &indices, std::vector<Texture*> &textures);
};

DELTA_ENGINE_NS_END
