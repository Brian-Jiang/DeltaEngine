---
name: slang-numeric-generics
description: Select and use Slang's capability-oriented numeric interfaces when writing or porting generic numeric code. Use for scalar-versus-shaped constraints, dot products, component masks, scalar splats, builtin conversion, wrapper or custom-number conformances, compound arithmetic, elementary functions, or builtin-representation constraints.
license: Apache-2.0 WITH LLVM-exception
compatibility: Requires a Slang build that provides the experimental slang.numerics module; compile with -experimental-feature.
---

# Use Slang Numeric Generics

Use the public numeric hierarchy from Slang's experimental numerics module.
It separates scalar algebra from logical shape: scalars, vectors, and matrices can share component-wise capabilities, while `IScalar...` refinements exclude shaped values.

## Enable the module

Add this import to every source module that directly names these interfaces:

```slang
import slang.numerics;
```

The compiler invocation must also include `-experimental-feature`.
If the surrounding build system owns compiler options, add the source import and use the provided command rather than inventing a second compile path.

Import `slang.numerics.differentiable` instead when generic operations must participate in Slang automatic differentiation.
That module re-exports the base numerics definitions.
Use `IDifferentiableDotProduct` when a generic dot product must participate in automatic differentiation.
Built-in floating-point scalars and ordinary floating-point vectors already provide that conformance.
For a user-defined type, `IDifferentiableDotProduct` is not just a marker: it requires the type's usual `IDifferentiable` witnesses plus explicit forward- and reverse-mode derivative rules for `dot`.

If the import succeeds but documented interfaces are undefined, verify that the compiler executable and its standard-module artifacts come from the same compatible build.
Do not replace the interfaces with legacy constraints merely to work around a stale or mismatched module installation.

## Classify the operated type

Before choosing a constraint, identify:

1. The exact type on which each operator or intrinsic is invoked.
2. Whether that type must be a scalar, may itself be scalar-or-shaped, or constructs a builtin shape such as `vector<T, N>` from a scalar element parameter.
3. The smallest operation set used by that declaration's body.

Use an `IScalar...` refinement when the parameter denotes one logical scalar, including a parameter used as the element type of `vector<T, N>` or `matrix<T, R, C>`.
Use the non-scalar interface when the parameter itself may be a scalar, vector, or matrix and its operations are component-wise.

A constraint on an element or payload does not automatically provide operations on a constructed vector, matrix, array, or user wrapper.
Confirm the shaped type's conformance or constrain the type actually used by the operation.

## Select the narrowest useful capability

Choose the smallest public interface or conjunction that covers the body.
Escalate only when another operation requires it.

When the operation inventory is broad or incomplete during a port, use `IScalarReal` as the readable scalar default or `IReal` as the scalar-or-shaped default.
Narrow it once the contract is understood.

`IDotProduct` is independent of `INumeric` and `IReal`.
Conjoin it with the arithmetic capability when the same body uses both, for example `T : IReal & IDotProduct`.

Important distinctions include:

- `IAdditive` supplies addition, subtraction, and zero.
- `INumeric` adds same-type multiplication, one, and construction from builtin integer types.
- `ISignedNumeric` adds negation and absolute value.
- `IDotProduct` independently supplies `dot(left, right)`, returning `T.Scalar`; built-in numeric scalars and ordinary vectors conform, but matrices and cooperative vectors currently do not.
- `IFractional` adds division, reciprocal, and construction from builtin floating-point types without requiring an IEEE representation or elementary functions.
- `IFloatingPoint` adds representation-specific rounding, remainder, splitting, sign-copying, and classification.
- Elementary-function families are independent capabilities and can be joined with `&`.
- `IReal` is a convenience conjunction for fractional arithmetic, elementary functions, component-wise ordering, and real-ordering functions such as `min`, `max`, and `step`.
- `IComponentwiseOrdered` returns a same-shaped mask.
  Use `IPartiallyOrdered` or `ITotallyOrdered` when comparisons must return one scalar `bool`.

Read [references/interface-selection.md](references/interface-selection.md) for the full selection table.

## Preserve shape and conversions

`INumericShapedType` provides `T.Scalar`, `T.Mask`, and `T.fromScalar(value)`.
Use `T.fromScalar(scalar)` when the body needs a shaped value from one logical scalar.
Use `T.zero()` and `T.one()` through the appropriate arithmetic interfaces.

Public numeric interfaces deliberately support construction from builtin scalar types:

```slang
T addToDouble<T : IFractional>(T left, double right)
{
    return left + T(right);
}
```

Prefer `T(value)` through `INumeric` or `IFractional` over calling an internal conversion primitive in user-facing generic code.
When the source value has a generic builtin type, constrain that source with `IBuiltinScalarIntegerType` or `IBuiltinScalarFloatingPointType`.
These public aliases describe both the compiler-supported source representation and its scalar capability.

Numeric conversion is not bit reinterpretation.
Never substitute `bit_cast`, a same-type copy, a default value, or one-component splatting for component-wise value conversion.
Read [references/shapes-and-conversions.md](references/shapes-and-conversions.md) for masks, splats, and conversions.

## Keep extensible and builtin-representation contracts distinct

The public `IBuiltinScalar...` aliases combine an extensible `IScalar...` capability with the compiler-supported builtin representation domain.
They intentionally exclude user-defined numeric types, but unlike the implementation-level `__Builtin...` markers they are user-facing complete constraints.

Use an `IBuiltinScalar...` constraint only when the implementation genuinely depends on a builtin representation, intrinsic, or shape constructor.
For example, an explicit real-valued `vector<T, N>` can justify `T : IBuiltinScalarReal`.
If no convenience alias includes an independent operation family, conjoin it explicitly, as in `IBuiltinScalarFloatingPointType & ITrigonometricFunctions`.

Do not spell implementation-level `__Builtin...` markers in user-facing code when a corresponding `IBuiltinScalar...` alias expresses the contract.
Do not redundantly repeat the scalar capability already included in an `IBuiltinScalar...` alias.

Do not fall back to legacy `IArithmetic`, `IFloat`, or `IComparable` when the capability-oriented interfaces express the contract.
Read [references/public-versus-builtin.md](references/public-versus-builtin.md) when a builtin-representation constraint appears necessary.

## Conform user-defined numeric types deliberately

Make a custom type conform only when the type itself is passed to generic code requiring that capability.
A dual number, complex number, interval, or similar mathematical scalar should normally start with a scalar refinement such as `IScalarAdditive`, `IScalarFractional`, or a conjunction of the exact capabilities it implements.

Add `IDotProduct` only when the custom type itself has a clear same-shape inner-product operation needed by downstream generic code.
Its requirement is `Scalar dotProductWith(This other)`; it is independent of ordinary multiplication and does not imply matrix semantics.

When generic automatic-differentiation code needs `dot` on the custom type, refine that conformance to `IDifferentiableDotProduct`.
In addition to the base `dotProductWith` operation, implement `forwardDifferentiateDotProduct` and `backwardDifferentiateDotProduct`; Slang cannot currently infer or attach those derivative rules from the interface requirement alone.

Implement `Scalar == This`, `Mask == bool`, `fromScalar`, and every operation inherited by the chosen refinement.
Construction requirements from builtin integer and floating-point types are part of `INumeric` and `IFractional` respectively.
Do not claim `IFloatingPoint` merely because the custom value contains floating-point fields; that interface describes IEEE-like representation operations on the custom type itself.

Read [references/custom-numeric-types.md](references/custom-numeric-types.md) before adding a conformance to a wrapper or custom number.

## Probe before a large rewrite

Compile a small file containing the exact proposed import, constraint, operator, intrinsic, and representative instantiation.
For an explicitly constructed vector or matrix, probe both the public scalar capability and any justified builtin-element requirement.

Use this bounded repair order:

1. Confirm that `slang.numerics` is imported and experimental features are enabled by the build.
2. Confirm that the constraint applies to the type on which the operation is invoked.
3. Select the minimum public capability for the operation.
4. Add an `IBuiltinScalar...` requirement only for an actual builtin representation or builtin-only intrinsic.
5. If the source operation is provably component-wise and only a scalar overload exists, introduce one narrow same-shaped adapter that preserves the operation, shape, and element order.
6. If faithful conversion, reduction, mask, or shape-rebinding semantics cannot be expressed, preserve the coherent port and report the minimized compiler or library gap.

Do not replace floating-point `min` or `max` with a comparison ternary without verifying NaN semantics.
Do not reduce a comparison mask to `bool` unless the source algorithm requires a reduction.
Do not scalarize a vector or matrix merely to silence a diagnostic.
