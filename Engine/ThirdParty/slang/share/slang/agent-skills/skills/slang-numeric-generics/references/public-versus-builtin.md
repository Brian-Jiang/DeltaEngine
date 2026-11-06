# Extensible Numeric Contracts versus Builtin Representation Constraints

Public numeric interfaces describe capabilities that user-defined types can implement.
Public `IBuiltinScalar...` aliases combine those capabilities with compiler-recognized representation domains and intentionally cannot be satisfied by user types.

Use a public contract when ordinary interface operations express the body:

```slang
import slang.numerics;

T twice<T : IAdditive>(T value)
{
    return value + value;
}
```

This works for builtin additive values and for a user-defined type with a faithful `IAdditive` conformance.

This replacement is not equivalent:

```slang
T twiceBuiltin<T : IBuiltinScalarAdditive>(T value)
{
    return value + value;
}
```

`twiceBuiltin` excludes every user-defined numeric type, even if it implements exactly the addition operation the body needs.

Use an `IBuiltinScalar...` constraint when the implementation depends on a builtin-only intrinsic, representation, layout, or type constructor.
For a generic builtin vector element, select the alias matching the scalar capability the body needs, such as `IBuiltinScalarReal`.
If the body also needs an independent operation family, conjoin that interface explicitly.

Do not use the implementation-level `__Builtin...` interfaces in user-facing code when a public alias covers the contract.
For example, prefer `IBuiltinScalarFloatingPointType` over `__BuiltinFloatingPointType & IScalarFloatingPoint`.

The compiler-checked [negative example](examples/sealed-builtin-negative.slang) confirms that a user-defined additive type cannot satisfy the builtin-representation constraint.
