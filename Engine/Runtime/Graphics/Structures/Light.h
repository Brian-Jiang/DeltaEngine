#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>
#include <minwindef.h>

DELTA_ENGINE_NS_BEGIN

struct alignas(16) LightCB
{
    /// Number of directional lights uploaded this frame.
    UINT numDirectionalLights;

    /// Number of point lights uploaded this frame.
    UINT numPointLights;

    /// Number of spot lights uploaded this frame.
    UINT numSpotLights;
};

struct alignas(16) DirectionalLightBuffer
{
    /// Normalized light direction.
    DirectX::XMVECTOR direction;

    /// Light color in linear space.
    DirectX::XMVECTOR color;

    /// Light intensity multiplier.
    float intensity;

    DirectX::XMMATRIX lightViewProj;
    DirectX::XMFLOAT4 atlasUVRect;
    float shadowBias;
    float pcssLightSize;
    int shadowEnabled;
    float shadowNormalBias;
    float shadowSlopeBias;
    float _pad0;
    float _pad1;
    float _pad2;
};

struct alignas(16) PointLightBuffer
{
    /// Light world position.
    DirectX::XMVECTOR position;

    /// Light color in linear space.
    DirectX::XMVECTOR color;

    /// Light intensity multiplier.
    float intensity;

    /// Maximum light range.
    float range;

    int cubeArrayIndex;
    float shadowBias;
    float pcssLightSize;
    int shadowEnabled;
    float shadowNearZ;
    float shadowNormalBias;
    float shadowSlopeBias;
    float _pad1;
    float _pad2;
    float _pad3;
};

struct alignas(16) SpotLightBuffer
{
    /// Light world position.
    DirectX::XMVECTOR position;

    /// Normalized light direction.
    DirectX::XMVECTOR direction;

    /// Light color in linear space.
    DirectX::XMVECTOR color;

    /// Light intensity multiplier.
    float intensity;

    /// Maximum light range.
    float range;

    /// Inner cone angle in radians.
    float innerConeAngle;

    /// Outer cone angle in radians.
    float outerConeAngle;

    DirectX::XMMATRIX lightViewProj;
    DirectX::XMFLOAT4 atlasUVRect;
    float shadowBias;
    float pcssLightSize;
    int shadowEnabled;
    float shadowNormalBias;
    float shadowSlopeBias;
    float _pad0;
    float _pad1;
    float _pad2;
};

struct alignas(16) Light
{
    /// Light world position.
    DirectX::XMVECTOR position;

    /// Light color in linear space.
    DirectX::XMVECTOR color;

    /// Light intensity multiplier.
    float intensity;
};

DELTA_ENGINE_NS_END
