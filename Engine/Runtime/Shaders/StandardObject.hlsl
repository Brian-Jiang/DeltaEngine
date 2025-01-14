#pragma once

#include "StandardConstantStructs.hlsl"
#include "StandardLighting.hlsl"


struct StandardObjectVSInput
{
    float4 position : POSITION;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct StandardObjectPSInput
{
    float4 position : SV_POSITION;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
};

StandardObjectPSInput VSMain(StandardObjectVSInput input)
{
    StandardObjectPSInput result;
    
    //result.position = position;
    //position = mul(modelMatrix, position);
    //float4 position = float4(input.position, 1.0f);
    float4 worldPosition = mul(PerObjectCB.worldMatrix, input.position);
    float4 viewPosition = mul(PerObjectCB.viewMatrix, worldPosition);
    float4 cameraPosition = mul(PerObjectCB.projectionMatrix, viewPosition);
    
    result.position = cameraPosition;
    //result.color = input.color * float4(input.instanceColor, 1.0);
    //result.color = float4(position.z / 10.0f, 0.0f, 0.0f, 1.0f);
    result.normal = input.normal;
    result.uv = input.uv;

    return result;
}

float4 PSMain(StandardObjectPSInput input) : SV_TARGET
{
    float4 normal = normalize(input.normal);
    Texture2D diffuseTex = StandardTextures[PerObjectCB.diffuseTexId];
    float4 diffuseColor = diffuseTex.Sample(StandardSamplers[PerObjectCB.diffuseTexId], input.uv);
    float4 color = diffuseColor * PerObjectCB.color;
    SurfaceInfo surface;
    surface.position = input.position;
    surface.normal = normal;
    surface.diffuseColor = color;
    surface.specularColor = float4(1.0f, 1.0f, 1.0f, 32.0f);
    
    float4 cameraPosition = PerObjectCB.cameraPosition;
    float4 lightingColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    lightingColor += CalcLight(surface, PerObjectCB.light0, cameraPosition);
    lightingColor += CalcLight(surface, PerObjectCB.light1, cameraPosition);
    lightingColor += CalcLight(surface, PerObjectCB.light2, cameraPosition);
    lightingColor += CalcLight(surface, PerObjectCB.light3, cameraPosition);
    
    //float4 textureColor = g_texture.Sample(g_sampler, input.uv);
    //textureColor = float4(1.0, 1.0, 1.0, 1.0);
    
    //float falloff = 1.0f / (lightDistance * lightDistance);
    //falloff = 1.0f;
    ////return float4(left, 1.0f);
    
    
    
    return lightingColor;
    
    
    //return float4(input.uv, 0, 1); // Debugging (show UVs as colors)
    //float2 uv;
    //uv.x = input.uv.x;
    ////uv.x = (input.uv.x + 1.0) / 2.0;
    ////uv.y = 1 - input.uv.y;  // Flip UVs (DirectX vs OpenGL)
    //uv.y = input.uv.y;
    ////uv.y = (input.uv.y + 1.0) / 2.0;
    
    
    //if (textureColor.a < 0.1f)
    //{
    //    discard;
    //}
    
    // textureColor.a = 1.0f;
    // return textureColor * float4(input.uv, 0.0f, 1.0f);
    //return textureColor;
    //return float4(textureColor.rgb, textureColor.a);
}