// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

int sumSupported<int N>(int values[N])
{
    static_assert(N >= 1 && N <= 3, "supported extents are 1, 2, and 3");
    int result = 0;
    for (int index = 0; index < N; ++index)
        result += values[index];
    return result;
}

RWStructuredBuffer<int> outputBuffer;

[shader("compute")]
[numthreads(1, 1, 1)]
void main()
{
    int values[3] = {1, 2, 4};
    outputBuffer[0] = sumSupported<3>(values);
}
