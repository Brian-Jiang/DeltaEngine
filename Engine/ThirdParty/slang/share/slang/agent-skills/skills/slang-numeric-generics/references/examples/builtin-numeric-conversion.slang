// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

import slang.numerics;

T addBuiltinFloat<T : IFractional, S : IBuiltinScalarFloatingPointType>(T left, S right)
{
    return left + T(right);
}

vector<TargetScalar, N> convertBuiltinVector<
    TargetScalar : IBuiltinScalarFractional,
    SourceScalar : IBuiltinScalarFloatingPointType,
    int N>(vector<SourceScalar, N> source)
{
    vector<TargetScalar, N> result;
    [unroll]
    for (int component = 0; component < N; ++component)
    {
        result[component] = TargetScalar(source[component]);
    }
    return result;
}

RWStructuredBuffer<float4> outputBuffer;

[shader("compute")]
[numthreads(1, 1, 1)]
void main()
{
    vector<half, 3> source = vector<half, 3>(half(1.0), half(2.0), half(3.0));
    vector<float, 3> converted = convertBuiltinVector<float, half, 3>(source);
    outputBuffer[0] = float4(converted, addBuiltinFloat(1.0, half(0.5)));
}
