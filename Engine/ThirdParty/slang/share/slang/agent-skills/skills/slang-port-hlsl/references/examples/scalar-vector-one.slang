// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

static const uint kCount = 2;

float sumVectorElements(vector<float, 1> values[kCount])
{
    return values[0].x + values[1].x;
}

RWStructuredBuffer<float> outputBuffer;

[shader("compute")]
[numthreads(1, 1, 1)]
void main()
{
    float scalars[kCount] = {2.0, 3.0};
    vector<float, 1> vectors[kCount];
    for (uint index = 0; index < kCount; ++index)
    {
        vectors[index] = vector<float, 1>(scalars[index]);
    }
    outputBuffer[0] = sumVectorElements(vectors);
}
