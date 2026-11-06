// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

import slang.numerics;

struct Counter : IScalarAdditive
{
    int value;

    typealias Scalar = Counter;
    typealias Mask = bool;

    static This fromScalar(Scalar value) { return value; }
    static This zero() { return {0}; }
    This plus(This other) { return {value + other.value}; }
    This minus(This other) { return {value - other.value}; }
}

T twiceBuiltin<T : IBuiltinScalarAdditive>(T value)
{
    return value + value;
}

void main()
{
    Counter value = {7};
    Counter result = twiceBuiltin(value);
}
