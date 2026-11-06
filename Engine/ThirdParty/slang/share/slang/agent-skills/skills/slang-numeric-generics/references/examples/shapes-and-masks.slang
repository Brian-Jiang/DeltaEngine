// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

import slang.numerics;

T weightedSum<T : IFractional>(T left, T right, T.Scalar weight)
{
    T shapedWeight = T.fromScalar(weight);
    return left * (T.one() - shapedWeight) + right * shapedWeight;
}

T.Mask componentLess<T : IComponentwiseOrdered>(T left, T right)
{
    return left < right;
}

bool allComponentsLess<T : IComponentwiseOrdered>(T left, T right)
{
    return (left < right).all();
}

RWStructuredBuffer<float4> outputBuffer;

[shader("compute")]
[numthreads(1, 1, 1)]
void main()
{
    float3 value = weightedSum(float3(1.0), float3(3.0), 0.25);
    vector<bool, 3> mask = componentLess(value, float3(4.0));
    outputBuffer[0] = float4(
        value,
        mask.all() && allComponentsLess(value, float3(4.0)) ? 1.0 : 0.0);
}
