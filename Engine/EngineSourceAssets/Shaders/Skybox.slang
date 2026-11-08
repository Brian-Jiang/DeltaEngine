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
    float4 curClip : TEXCOORD1;
    float4 prevClip : TEXCOORD2;
    float4 position : SV_POSITION;
};

struct PSOut
{
    float4 color : SV_Target0;
    float2 motion : SV_Target1;
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
    // w = 0 drops translation: sky motion comes from camera rotation only.
    o.curClip = mul(ProjectionMatrixUnjittered, viewDir);
    o.prevClip = mul(PrevViewProjectionMatrix, float4(localPos, 0.0f));
    return o;
}

[shader("fragment")]
PSOut PSMain(VSOut input)
{
    PSOut o;
    o.color = g_skybox.Sample(g_sampler, input.texCoord);
    o.motion = (input.curClip.xy / input.curClip.w - input.prevClip.xy / input.prevClip.w) * float2(0.5, -0.5);
    return o;
}
