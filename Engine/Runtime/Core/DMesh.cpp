#include "Core/DMesh.h"

using namespace DeltaEngine;

DMesh::DMesh() {}

DMesh::DMesh(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices,
    std::shared_ptr<DMaterial>& material)
    : m_vertices(vertices)
    , m_indices(indices)
    , m_materials(material)
{
    
}

