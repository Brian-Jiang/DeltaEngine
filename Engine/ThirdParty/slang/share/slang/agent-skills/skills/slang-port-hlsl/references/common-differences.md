# Common HLSL-to-Slang Differences

Use this reference after a minimized diagnostic identifies a general language difference rather than a template-specific problem.

## Language selection and scope

Pass `-lang slang` when the source extension would otherwise select HLSL mode.
Slang mode follows lexical loop-variable scoping; do not rename a later variable merely to accommodate a legacy HLSL compatibility rule unless the chosen compilation mode actually requires it.

## Methods and mutation

Slang does not accept every C++-style out-of-line member-definition pattern used in HLSL.
Move a body into the type or an extension while preserving its receiver, constraints, and visibility.

Mark instance methods that modify value-type state as `[mutating]`.
The annotation belongs on both an interface requirement and its implementation.
Do not add it to a read-only operation simply because the underlying storage is reference-backed.

## Subscripts and writeback

Translate user-defined indexing to `__subscript` and choose accessors that match the original read/write contract.
Resource types and value arrays have different mutation models; do not force them through one mutable interface without confirming that receiver and writeback semantics agree.

An `inout` argument must designate writable storage of the required type.
When a swizzle, property, or converted expression does not, use a temporary and write it back explicitly if that preserves the source behavior.

## Scalars and one-component vectors

Do not assume that a conversion between `T` and `vector<T, 1>` is lifted through an array, resource, wrapper, or generic type argument.
For example, `float[Count]` does not satisfy a parameter of type `vector<float, 1>[Count]` merely because an individual `float` can initialize a one-component vector.

When storage identity is not required, adapt each element explicitly while preserving order and write back modified values when necessary.
When the source depends on aliasing, `inout`, or an exact layout, prefer an exact overload or report the type-system gap rather than treating the aggregates as interchangeable.

See the compiler-checked [one-component vector example](examples/scalar-vector-one.slang).

## Declarations and constants

A forward declaration followed by a full definition can be diagnosed as a conflicting declaration in native Slang.
Remove the forward declaration only when it is redundant and no ordering dependency remains.

Do not assume an HLSL `static const` aggregate is a compile-time Slang value.
Resource handles and other runtime-bound values can require construction at the use site or a different ownership model.
