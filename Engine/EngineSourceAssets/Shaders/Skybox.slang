// Procedural skybox: no vertex buffer, geometry from SV_VertexID.
// Binds to the shared root signature:
//   b0 = CameraCB (inline CBV)
//   t0 space1 = TextureCube
//   s0 = static anisotropic sampler

cbuffer CameraCB : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4x4 InvViewProjectionMatrix;
    float4 CameraPosition;
    float4x4 ProjectionMatrixUnjittered;
    float4x4 PrevViewProjectionMatrix;
    float2 Jitter;
    float2 JitterPad;
}

TextureCube g_skybox : register(t0, space1);
SamplerState g_sampler : register(s0);

static const float3 kCubeVerts[36] =
{
    { 1,-1, 1}, { 1,-1,-1}, { 1, 1,-1},
    { 1,-1, 1}, { 1, 1,-1}, { 1, 1, 1},
    {-1,-1,-1}, {-1,-1, 1}, {-1, 1, 1},
    {-1,-1,-1}, {-1, 1, 1}, {-1, 1,-1},
    {-1, 1, 1}, { 1, 1, 1}, { 1, 1,-1},
    {-1, 1, 1}, { 1, 1,-1}, {-1, 1,-1},
    {-1,-1,-1}, { 1,-1,-1}, { 1,-1, 1},
    {-1,-1,-1}, { 1,-1, 1}, {-1,-1, 1},
    {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1},
    {-1,-1, 1}, { 1, 1, 1}, {-1, 1, 1},
    { 1,-1,-1}, {-1,-1,-1}, {-1, 1,-1},
    { 1,-1,-1}, {-1, 1,-1}, { 1, 1,-1},
};

struct VSOut
{
    float3 texCoord : TEXCOORD;
    float4 position : SV_POSITION;
};

[shader("vertex")]
VSOut VSMain(uint vid : SV_VertexID)
{
    float3 localPos = kCubeVerts[vid];

    float4 viewDir = mul(ViewMatrix, float4(localPos, 0.0f));
    float4 clipPos = mul(ProjectionMatrix, viewDir);

    VSOut o;
    o.position = clipPos.xyww;
    o.texCoord = localPos;
    return o;
}

[shader("fragment")]
float4 PSMain(VSOut input) : SV_TARGET
{
    return g_skybox.Sample(g_sampler, input.texCoord);
}
