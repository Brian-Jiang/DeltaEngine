// Skybox.hlsl
// Procedural skybox: no vertex buffer, geometry from SV_VertexID.
// Binds to the shared root signature:
//   b0 = CameraCB (inline CBV)
//   t0 space1 = TextureCube (Texture descriptor table)
//   s0 = static anisotropic sampler

cbuffer CameraCB : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4   CameraPosition;
}

TextureCube  g_skybox  : register(t0, space1);
SamplerState g_sampler : register(s0);

// 36 vertices: 6 faces x 2 triangles x 3 vertices.
// Winding is CCW when viewed from inside the cube (camera is inside).
static const float3 kCubeVerts[36] =
{
    // +X
    { 1,-1, 1}, { 1,-1,-1}, { 1, 1,-1},
    { 1,-1, 1}, { 1, 1,-1}, { 1, 1, 1},
    // -X
    {-1,-1,-1}, {-1,-1, 1}, {-1, 1, 1},
    {-1,-1,-1}, {-1, 1, 1}, {-1, 1,-1},
    // +Y
    {-1, 1, 1}, { 1, 1, 1}, { 1, 1,-1},
    {-1, 1, 1}, { 1, 1,-1}, {-1, 1,-1},
    // -Y
    {-1,-1,-1}, { 1,-1,-1}, { 1,-1, 1},
    {-1,-1,-1}, { 1,-1, 1}, {-1,-1, 1},
    // +Z
    {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1},
    {-1,-1, 1}, { 1, 1, 1}, {-1, 1, 1},
    // -Z
    { 1,-1,-1}, {-1,-1,-1}, {-1, 1,-1},
    { 1,-1,-1}, {-1, 1,-1}, { 1, 1,-1},
};

struct VSOut
{
    float3 texCoord : TEXCOORD;
    float4 position : SV_POSITION;
};

VSOut VSMain(uint vid : SV_VertexID)
{
    float3 localPos = kCubeVerts[vid];

    // w=0 passes direction (not position) through the view matrix,
    // eliminating camera translation while keeping rotation.
    // Matches the row-vector convention used by the rest of the engine
    // (mul(vec, matrix) = row-vector x matrix).
    float4 viewDir = mul(float4(localPos, 0.0f), ViewMatrix);
    float4 clipPos = mul(viewDir, ProjectionMatrix);

    VSOut o;
    // xyww: output.z = clipPos.w -> NDC depth = w/w = 1.0 (far plane).
    // Skybox wins depth test only where no opaque geometry was rendered.
    o.position = clipPos.xyww;
    o.texCoord = localPos;
    return o;
}

float4 PSMain(VSOut input) : SV_TARGET
{
    return g_skybox.Sample(g_sampler, input.texCoord);
}
