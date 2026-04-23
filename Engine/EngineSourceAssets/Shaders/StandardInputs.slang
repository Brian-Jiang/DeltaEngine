module StandardInputs;

public struct StandardVSInput
{
    public float3 position : POSITION;
    public float4 color : COLOR;
    public float3 normal : NORMAL;
    public float2 uv : TEXCOORD;
};

public struct StandardPSInput
{
    public float4 color : COLOR;
    public float3 normal : NORMAL;
    public float2 uv : TEXCOORD;
    public float3 worldPosition : TEXCOORD1;
    public float4 position : SV_POSITION;
};
