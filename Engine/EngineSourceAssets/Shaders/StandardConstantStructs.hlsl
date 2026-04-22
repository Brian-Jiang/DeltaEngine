#pragma once

#include "StandardLighting.hlsl"

struct GlobalConstant
{
    
};

ConstantBuffer<GlobalConstant> GlobalCB : register(b0);

struct PerFrameConstant
{
    float gameTime;
    uint gameFrame;
    uint screenWidth;
    uint screenHeight;
};

ConstantBuffer<PerFrameConstant> PerFrameCB : register(b1);

struct PerObjectConstant
{
    float4x4 worldMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4 cameraPosition;
    
    float4 color;
    Light light0;
    Light light1;
    Light light2;
    Light light3;

    uint diffuseTexId;
    uint normalTexId;
};

ConstantBuffer<PerObjectConstant> PerObjectCB : register(b2);

struct PerInstanceData
{
    float4x4 worldMatrix;
};

StructuredBuffer<PerInstanceData> PerInstanceDataBuffer : register(t0);

Texture2D StandardTextures[] : register(t1);
SamplerState StandardSamplers[] : register(s0);