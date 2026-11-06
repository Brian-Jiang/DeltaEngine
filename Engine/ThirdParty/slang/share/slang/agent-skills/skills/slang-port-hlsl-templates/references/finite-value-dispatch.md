# Finite Value-Dependent Dispatch

Use these patterns when an HLSL template calls an overload family whose parameter types differ by a finite value such as an array extent.

## One generic implementation

When the overloads are instances of one algorithm, replace the family with a value-generic callee:

```slang
int sumSupported<int N>(int values[N])
{
    static_assert(N >= 1 && N <= 3, "supported extents are 1, 2, and 3");
    int result = 0;
    for (int index = 0; index < N; ++index)
        result += values[index];
    return result;
}
```

Do not change a finite `N = 1, 2, 3` contract into an unbounded loop unless the source contract was already unbounded.
Use the generic loop only when it is equivalent to every source overload.

Replace or rename the original overload family coherently.
Adding an overlapping catch-all generic beside concrete overloads with the same name can make calls ambiguous or cause a wrapper to call itself.
When the original names must remain, give the new shared implementation a distinct helper name and make each public overload delegate to it explicitly.

## Specialized conformance dispatch

When the implementations are materially different, use a dispatcher with one conformance per supported value:

```slang
interface IArrayCase<int N>
{
    static int apply(int values[N]);
}

struct ArrayCase<int N> {}

extension ArrayCase<1> : IArrayCase<1>
{
    static int apply(int values[1]) { return values[0]; }
}

extension ArrayCase<2> : IArrayCase<2>
{
    static int apply(int values[2]) { return values[0] - values[1]; }
}

int applySupported<int N>(int values[N])
    where ArrayCase<N> : IArrayCase<N>
{
    return ArrayCase<N>.apply(values);
}
```

The caller's conformance constraint preserves the finite domain.

## What does not refine a dependent type

This branch does not make `values` become `int[1]`:

```slang
int invalidDispatch<int N>(int values[N])
{
    if (N == 1)
        return onlyOne(values); // `values` still has type `int[N]`.
    return 0;
}
```

Do not add casts or copy only one element merely to force the call to resolve.
Choose one of the faithful patterns above or report the minimized blocker.
See the complete compiler-checked [generic](examples/finite-value-generic.slang) and [specialized](examples/finite-value-specialized.slang) examples.
