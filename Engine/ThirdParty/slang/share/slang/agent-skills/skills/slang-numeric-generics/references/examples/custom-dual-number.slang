// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

import slang.numerics;

struct Dual : IScalarFractional
{
    float primal;
    float tangent;

    typealias Scalar = Dual;
    typealias Mask = bool;

    __init<TSource : IBuiltinScalarIntegerType>(TSource value)
    {
        primal = __realCast<float>(value);
        tangent = 0.0;
    }

    __init<TSource : IBuiltinScalarFloatingPointType>(TSource value)
    {
        primal = __realCast<float>(value);
        tangent = 0.0;
    }

    __init(float primal, float tangent)
    {
        this.primal = primal;
        this.tangent = tangent;
    }

    static This fromScalar(Scalar value)
    {
        return value;
    }

    static This zero()
    {
        return Dual(0.0, 0.0);
    }

    static This one()
    {
        return Dual(1.0, 0.0);
    }

    This plus(This other)
    {
        return Dual(primal + other.primal, tangent + other.tangent);
    }

    This minus(This other)
    {
        return Dual(primal - other.primal, tangent - other.tangent);
    }

    This multipliedBy(This other)
    {
        return Dual(primal * other.primal, tangent * other.primal + primal * other.tangent);
    }

    This dividedBy(This other)
    {
        float denominator = other.primal * other.primal;
        return Dual(
            primal / other.primal,
            (tangent * other.primal - primal * other.tangent) / denominator);
    }

    This negated()
    {
        return Dual(-primal, -tangent);
    }

    This absoluteValue()
    {
        return primal < 0.0 ? Dual(-primal, -tangent) : this;
    }

    This reciprocal()
    {
        return Dual(1.0 / primal, -tangent / (primal * primal));
    }
}

T rationalCurve<T : IScalarFractional>(T value)
{
    return (value * value + T(1)) / T(2);
}

RWStructuredBuffer<float2> outputBuffer;

[shader("compute")]
[numthreads(1, 1, 1)]
void main()
{
    Dual input = Dual(3.0, 1.0);
    Dual output = rationalCurve(input);
    outputBuffer[0] = float2(output.primal, output.tangent);
}
