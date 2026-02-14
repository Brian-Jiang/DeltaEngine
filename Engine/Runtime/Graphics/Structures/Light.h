#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>

DELTA_ENGINE_NS_BEGIN

struct alignas(16) LightCB
{
    UINT numDirectionalLights;
    UINT numPointLights;
    UINT numSpotLights;
};

struct alignas(16) DirectionalLightBuffer
{
    DirectX::XMVECTOR direction;
    DirectX::XMVECTOR color;
    float intensity;
};

struct alignas(16) PointLightBuffer
{
    DirectX::XMVECTOR position;
    DirectX::XMVECTOR color;
    float intensity;
    float range;
};

struct alignas(16) SpotLightBuffer
{
    DirectX::XMVECTOR position;
    DirectX::XMVECTOR direction;
    DirectX::XMVECTOR color;
    float intensity;
    float range;
    float innerConeAngle; // in radians
    float outerConeAngle; // in radians
};

struct alignas(16) Light
{
    DirectX::XMVECTOR position;
    DirectX::XMVECTOR color;
    float intensity;
};

DELTA_ENGINE_NS_END