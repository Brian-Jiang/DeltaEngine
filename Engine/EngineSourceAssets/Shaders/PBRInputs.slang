module PBRInputs;

public struct PBRVSInput
{
    public float3 position : POSITION;
    public float4 color    : COLOR;
    public float3 normal   : NORMAL;
    public float3 tangent  : TANGENT;
    public float2 uv       : TEXCOORD;
};

public struct PBRPSInput
{
    public float4 color      : COLOR;
    public float3 normalWS   : NORMAL;
    public float4 tangentWS  : TANGENT;
    public float2 uv         : TEXCOORD;
    public float3 positionWS : TEXCOORD1;
    public float4 position   : SV_POSITION;
};
