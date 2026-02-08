#pragma once

#include "EngineIncludes.h"

#include <vector>

#include "Runtime/Core/DObject.h"
#include "Graphics/Structures/Vertex.h"
#include "Graphics/DTexture.h"

DELTA_ENGINE_NS_BEGIN

class Mesh : public DObject
{
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<std::shared_ptr<DTexture>> textures;

    Mesh();
    Mesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, std::vector<std::shared_ptr<DTexture>>& textures);
};

DELTA_ENGINE_NS_END
