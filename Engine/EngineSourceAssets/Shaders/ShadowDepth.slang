struct ShadowVSInput
{
    float3 position : POSITION;
};

ConstantBuffer<float4x4> ShadowViewProj : register(b0);
ConstantBuffer<float4x4> ShadowWorldMatrix : register(b1);

[shader("vertex")]
float4 VSMain(ShadowVSInput input) : SV_Position
{
    float4 worldPos = mul(ShadowWorldMatrix, float4(input.position, 1.0f));
    return mul(ShadowViewProj, worldPos);
}
