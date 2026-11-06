// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

import slang.numerics;

struct Pair<T : IScalarAdditive> : IAdditive
{
    T x;
    T y;

    typealias Scalar = T;
    typealias Mask = vector<bool, 2>;

    static This fromScalar(Scalar value)
    {
        return {value, value};
    }

    static This zero()
    {
        return {T.zero(), T.zero()};
    }

    This plus(This other)
    {
        return {x + other.x, y + other.y};
    }

    This minus(This other)
    {
        return {x - other.x, y - other.y};
    }
}

T addValues<T : IAdditive>(T left, T right)
{
    return left + right;
}

RWStructuredBuffer<float2> outputBuffer;

[shader("compute")]
[numthreads(1, 1, 1)]
void main()
{
    Pair<float> left = {1.0, 2.0};
    Pair<float> right = {3.0, 5.0};
    Pair<float> result = addValues(left, right);
    outputBuffer[0] = float2(result.x, result.y);
}
