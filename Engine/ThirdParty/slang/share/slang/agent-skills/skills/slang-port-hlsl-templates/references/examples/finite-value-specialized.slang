// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

interface IArrayCase<int N>
{
    static int apply(int values[N]);
}

struct ArrayCase<int N>
{
}

extension ArrayCase<1> : IArrayCase<1>
{
    static int apply(int values[1])
    {
        return values[0];
    }
}

extension ArrayCase<2> : IArrayCase<2>
{
    static int apply(int values[2])
    {
        return values[0] - values[1];
    }
}

int applySupported<int N>(int values[N])
    where ArrayCase<N> : IArrayCase<N>
{
    return ArrayCase<N>.apply(values);
}

RWStructuredBuffer<int> outputBuffer;

[shader("compute")]
[numthreads(1, 1, 1)]
void main()
{
    int values[2] = {7, 2};
    outputBuffer[0] = applySupported<2>(values);
}
