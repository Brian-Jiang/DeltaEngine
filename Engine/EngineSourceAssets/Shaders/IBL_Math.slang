module IBL_Math;

public static const float IBL_PI = 3.14159265358979f;

public float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

public float2 Hammersley(uint i, uint N)
{
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

public float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    float a = roughness * roughness;

    float phi      = 2.0 * IBL_PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    float3 up        = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent   = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

public float D_GGX_IBL(float NdotH, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (IBL_PI * d * d);
}

public float G_SchlickGGX_IBL(float NdotV, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

public float G_Smith_IBL(float NdotV, float NdotL, float roughness)
{
    return G_SchlickGGX_IBL(NdotV, roughness) * G_SchlickGGX_IBL(NdotL, roughness);
}

public float3 DirectionFromCubeFaceUV(uint face, float2 uv)
{
    float2 xy = uv * 2.0 - 1.0;
    float3 dir;
    switch (face)
    {
        case 0: dir = float3( 1.0, -xy.y, -xy.x); break;
        case 1: dir = float3(-1.0, -xy.y,  xy.x); break;
        case 2: dir = float3( xy.x,  1.0,  xy.y); break;
        case 3: dir = float3( xy.x, -1.0, -xy.y); break;
        case 4: dir = float3( xy.x, -xy.y,  1.0); break;
        default: dir = float3(-xy.x, -xy.y, -1.0); break;
    }
    return normalize(dir);
}
