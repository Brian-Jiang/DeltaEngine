#pragma once

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1

struct Light
{
    float4 position;
    float4 ambientColor;
    float4 diffuseColor;
    float4 specularColor;
    float4 direction;
    uint type;
};

struct SurfaceInfo
{
    float4 position;
    float4 normal;
    float4 diffuseColor;
    float4 specularColor;  // w is alpha
};

float4 CalcDirectionalLight(SurfaceInfo surface, Light light, float4 cameraPosition)
{
    float4 normal = normalize(surface.normal);
    float4 result = float4(0.0f, 0.0f, 0.0f, 1.0f);
    result += light.ambientColor * surface.diffuseColor;
    
    float4 left = normalize(-light.direction);
    float4 right = reflect(-left, normal);
    float4 view = normalize(cameraPosition - surface.position);
    result += light.diffuseColor * max(dot(normal, left), 0.0f) * surface.diffuseColor;
    
    float3 specular = light.specularColor.xyz * pow(max(dot(right, view), 0.0f), surface.specularColor.w) * surface.specularColor.xyz;
    result += float4(specular, 0.0f);
    
    return result;
}

float4 CalcPointLight(SurfaceInfo surface, Light light, float4 cameraPosition)
{
    float4 normal = normalize(surface.normal);
    float4 result = float4(0.0f, 0.0f, 0.0f, 1.0f);
    result += light.ambientColor * surface.diffuseColor;
    
    float4 left = normalize(light.position - surface.position);
    float4 right = reflect(-left, normal);
    float4 view = normalize(cameraPosition - surface.position);
    result += light.diffuseColor * max(dot(normal, left), 0.0f) * surface.diffuseColor;
    
    float3 specular = light.specularColor.xyz * pow(max(dot(right, view), 0.0f), surface.specularColor.w) * surface.specularColor.xyz;
    result += float4(specular, 0.0f);
    
    return result;
}

float4 CalcLight(SurfaceInfo surface, Light light, float4 cameraPosition)
{
    if (light.type == LIGHT_TYPE_DIRECTIONAL)
    {
        return CalcDirectionalLight(surface, light, cameraPosition);
    }
    else if (light.type == LIGHT_TYPE_POINT)
    {
        return CalcPointLight(surface, light, cameraPosition);
    }
    else
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
}
