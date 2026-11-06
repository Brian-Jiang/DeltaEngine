// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

import slang.numerics.differentiable;

struct Pair : IDifferentiableDotProduct
{
    float first;
    float second;

    typealias Differential = Pair;
    typealias Scalar = float;
    typealias Mask = bool;

    [Differentiable]
    static Differential dzero()
    {
        return {0.0, 0.0};
    }

    [Differentiable]
    static Differential dadd(Differential left, Differential right)
    {
        return {left.first + right.first, left.second + right.second};
    }

    [Differentiable]
    static This fromScalar(Scalar value)
    {
        return {value, value};
    }

    Scalar dotProductWith(This other)
    {
        return first * other.first + second * other.second;
    }

    static DifferentialPair<Scalar> forwardDifferentiateDotProduct(
        DifferentialPair<This> left,
        DifferentialPair<This> right)
    {
        return diffPair(
            left.p.dotProductWith(right.p),
            left.d.dotProductWith(right.p) + left.p.dotProductWith(right.d));
    }

    static void backwardDifferentiateDotProduct(
        inout DifferentialPair<This> left,
        inout DifferentialPair<This> right,
        Scalar.Differential outputDifferential)
    {
        let leftPrimal = left.p;
        let rightPrimal = right.p;
        left = diffPair(
            leftPrimal,
            Pair(
                rightPrimal.first * outputDifferential,
                rightPrimal.second * outputDifferential));
        right = diffPair(
            rightPrimal,
            Pair(
                leftPrimal.first * outputDifferential,
                leftPrimal.second * outputDifferential));
    }
}

[Differentiable]
float differentiablePairDot(Pair left, Pair right)
{
    return dot(left, right);
}

RWStructuredBuffer<float2> outputBuffer;

[shader("compute")]
[numthreads(1, 1, 1)]
void main()
{
    let result = fwd_diff(differentiablePairDot)(
        diffPair(Pair(1.0, 2.0), Pair(5.0, 0.0)),
        diffPair(Pair(3.0, 4.0), Pair(0.0, 7.0)));
    outputBuffer[0] = float2(result.p, result.d);
}
