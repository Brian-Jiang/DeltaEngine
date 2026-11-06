// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

import slang.numerics;

T squarePlusOne<T : INumeric>(T value)
{
    return value * value + T(1);
}

bool scalarLess<T : IScalarReal>(T left, T right)
{
    return left < right;
}

T shapedClamp<T : IReal>(T value, T lower, T upper)
{
    return clamp(value, lower, upper);
}

RWStructuredBuffer<float4> outputBuffer;

[shader("compute")]
[numthreads(1, 1, 1)]
void main()
{
    float3 value = shapedClamp(squarePlusOne(float3(1.0, 2.0, 3.0)), float3(0.0), float3(8.0));
    outputBuffer[0] = float4(value, scalarLess(1.0, 2.0) ? 1.0 : 0.0);
}
