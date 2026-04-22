Texture2D inputTex : register(t0);
SamplerState linearSampler : register(s0);

struct VSOut
{
    float4 pos : SV_Position;
    float2 texCoord : TEXCOORD0;
};

VSOut VSMain(uint id : SV_VertexID)
{
    float2 uv = float2((id << 1) & 2, id & 2);
    VSOut o;
    o.pos = float4(uv * 2.0f - 1.0f, 0.0f, 1.0f);
    o.texCoord = float2(uv.x, 1.0f - uv.y);
    return o;
}

float4 PSMain(VSOut input) : SV_Target
{
    return inputTex.Sample(linearSampler, input.texCoord);
}
