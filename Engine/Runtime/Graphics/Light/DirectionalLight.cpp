#include "DirectionalLight.h"

using namespace DeltaEngine;

DirectionalLight::DirectionalLight()
    : m_direction(0.0f, -1.0f, 0.0f, 0.0f)
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_intensity(1.0f)
{
}

DirectionalLight::~DirectionalLight()
{
}

void DirectionalLight::UpdateParameters(DirectX::XMVECTOR direction, DirectX::XMVECTOR color, float intensity)
{
    m_direction = direction;
    m_color = color;
    m_intensity = intensity;
}

void DirectionalLight::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context)
{
}
