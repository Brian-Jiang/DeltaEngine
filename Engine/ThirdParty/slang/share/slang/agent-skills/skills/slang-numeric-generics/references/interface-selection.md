# Numeric Interface Selection

Choose the smallest public capability that covers the generic body.
The scalar-or-shaped column accepts component-wise vector and matrix behavior where supported; the scalar-only column excludes those shapes.

| Body requirement | Scalar or shaped | Scalar only |
| --- | --- | --- |
| Shape metadata and scalar splat | `INumericShapedType` | `IScalarShapedType` |
| Addition, subtraction, zero, and compound assignment | `IAdditive` | `IScalarAdditive` |
| Same-type multiplication, one, and construction from builtin integers | `INumeric` | `IScalarNumeric` |
| Negation and absolute value | `ISignedNumeric` | `IScalarSignedNumeric` |
| Same-shape dot product returning the logical scalar | `IDotProduct` for scalars and ordinary vectors | `IDotProduct & IScalarShapedType` when scalar-only must be explicit |
| Division, reciprocal, and construction from builtin floating-point values | `IFractional` | `IScalarFractional` |
| Floating-point representation, rounding, and classification | `IFloatingPoint` | `IScalarFloatingPoint` |
| Integer division, remainder, bitwise operations, and shifts | `IIntegerType` | `IScalarIntegerType` |
| Signed or unsigned integer behavior | `ISignedIntegerType`, `IUnsignedIntegerType` | corresponding `IScalar...` refinement |
| Component-wise equality | `IComponentwiseEquatable` | `IEquatable` when a scalar `bool` is required |
| Component-wise relational comparison | `IComponentwiseOrdered` | `IPartiallyOrdered` or `ITotallyOrdered` |
| One elementary-function family | corresponding independent family | add `IScalarShapedType` if scalar-only |
| All elementary-function families | `IElementaryFunctions` | `IScalarElementaryFunctions` |
| Fractional arithmetic, elementary functions, component-wise ordering, `min`, `max`, and `step` | `IReal` | `IScalarReal` |

Use capability conjunctions when appropriate.
For example, `IFractional & IRootFunctions` is preferable to `IReal` when the body needs arithmetic and square roots but no ordering or other elementary functions.
Because `IDotProduct` is independent of the arithmetic hierarchy, a body that also uses real arithmetic should state `IReal & IDotProduct`.

`IFloatingPoint` is not a synonym for real-number operations, and `IReal` does not require an IEEE floating-point representation.
Use `IPartiallyOrdered` for IEEE-style scalar comparisons and `ITotallyOrdered` only when the type guarantees a total order.

When a type parameter is used as the element of an explicitly constructed builtin vector or matrix, select the scalar-only refinement from the table.
The builtin shape can additionally require a builtin-representation constraint.
For a real-valued vector element, use `T : IBuiltinScalarReal`.
The `IBuiltinScalar...` aliases are public conjunctions that preserve the readable scalar capability while imposing the compiler-supported representation domain.
For a built-in floating-point element plus one independent operation family, use a conjunction such as `IBuiltinScalarFloatingPointType & ITrigonometricFunctions`.

`IDotProduct` applies when the constrained type itself is the scalar or vector passed to `dot`.
Do not replace it with a scalar element constraint unless the generic signature explicitly constructs `vector<T, N>` and therefore operates on the element parameter separately.
Matrices and cooperative vectors do not currently conform to `IDotProduct`.
When `dot` must participate in automatic differentiation, import `slang.numerics.differentiable` and use `IDifferentiableDotProduct`.
Built-in floating-point scalars and ordinary floating-point vectors already conform; a custom type must provide explicit forward- and reverse-mode derivative rules in addition to its base dot product.
See the compiler-checked [builtin-vector example](examples/builtin-vector-element.slang), [custom dot-product example](examples/dot-product.slang), and [differentiable custom dot-product example](examples/differentiable-dot-product.slang).

Cooperative-vector conformances and other shaped-type support can differ from ordinary vectors and matrices.
Probe the exact shaped type and express only the independent capabilities needed by the algorithm.
