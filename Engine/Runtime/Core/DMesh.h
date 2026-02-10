#pragma once

#include "EngineIncludes.h"

#include <vector>
#include <memory>

#include "Core/DTexture.h"
#include "Graphics/Structures/Vertex.h"
#include "Runtime/Core/DObject.h"

DELTA_ENGINE_NS_BEGIN

class DMaterial;

class DMesh : public DObject
{
public:
    DMesh();
    DMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices, std::shared_ptr<DMaterial>& material);

    inline std::shared_ptr<DMaterial> GetMaterial() const { return m_materials; }

private:
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
    std::shared_ptr<DMaterial> m_materials;
};

DELTA_ENGINE_NS_END
