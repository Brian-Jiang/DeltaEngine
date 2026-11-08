module MotionVectors;

import StandardConstantStructs;

// Unjittered current-frame clip position of a world-space point.
public float4 CurrentUnjitteredClip(float4 worldPosition)
{
    return mul(CameraCB.projectionMatrixUnjittered, mul(CameraCB.viewMatrix, worldPosition));
}

// Previous-frame unjittered clip position of an object-space point.
public float4 PrevObjectClip(float4 localPosition)
{
    return mul(CameraCB.prevViewProjectionMatrix, mul(ObjectCB.prevWorldMatrix, localPosition));
}

// Motion in UV space: current UV - previous UV.
public float2 ComputeMotionUV(float4 curClip, float4 prevClip)
{
    float2 curNdc = curClip.xy / curClip.w;
    float2 prevNdc = prevClip.xy / prevClip.w;
    return (curNdc - prevNdc) * float2(0.5, -0.5);
}

public struct ForwardOutput
{
    public float4 color : SV_Target0;
    public float2 motion : SV_Target1;
};

public ForwardOutput MakeForwardOutput(float4 color, float4 curClip, float4 prevClip)
{
    ForwardOutput output;
    output.color = color;
    output.motion = ComputeMotionUV(curClip, prevClip);
    return output;
}
