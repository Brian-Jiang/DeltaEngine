// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

import slang.numerics;

struct Pair : IDotProduct
{
    float first;
    float second;

    typealias Scalar = float;
    typealias Mask = bool;

    static This fromScalar(Scalar value)
    {
        return {value, value};
    }

    Scalar dotProductWith(This other)
    {
        return first * other.first + second * other.second;
    }
}

T.Scalar genericDot<T : IDotProduct>(T left, T right)
{
    return dot(left, right);
}

RWStructuredBuffer<float4> outputBuffer;

[shader("compute")]
[numthreads(1, 1, 1)]
void main()
{
    float scalarResult = genericDot(2.0, 3.0);
    float vectorResult = genericDot(float3(1.0, 2.0, 3.0), float3(4.0, 5.0, 6.0));
    float customResult = genericDot(Pair(1.0, 2.0), Pair(3.0, 4.0));
    outputBuffer[0] = float4(scalarResult, vectorResult, customResult, 0.0);
}
