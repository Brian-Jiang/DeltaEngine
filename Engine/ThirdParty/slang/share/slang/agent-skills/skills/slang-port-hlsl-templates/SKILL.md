---
name: slang-port-hlsl-templates
description: Port HLSL C++-style template declarations and calls to native Slang generics while preserving type and value parameter domains. Use for template syntax, dependent extents, body-derived type contracts, or finite value-specialized overload families; use the numeric-generics skill when an inferred contract is numeric.
license: Apache-2.0 WITH LLVM-exception
---

# Port HLSL Templates to Slang Generics

Preserve the source algorithm and the full supported specialization domain.
Do not specialize only for visible call sites or delete dormant template declarations to make one entry point compile.

## Inventory the abstraction before editing

For every template declaration and call site, record:

- whether each parameter is a type or a value, and the type of each value parameter;
- dependent array extents, loop bounds, compile-time branches, and overload choices;
- operations required of each type parameter, including construction, copying, mutation, indexing, calls, operators, associated types or constants, and resource access;
- the intended specialization domain, including finite value cases not reached by the visible entry point.

Infer a type parameter's contract from the template body, documentation, and representative uses.
This is a porting analysis technique, not a request for the Slang compiler to infer generic constraints from a body.

## Port parameters directly when possible

Slang accepts type-first value-parameter syntax such as `<int N>`, `<uint N>`, and `<bool Enabled>`.
The equivalent `<let N : int>` spelling is also available.
Preserve dependent extents and compile-time values instead of replacing them with macros.

Leave a type parameter unconstrained when its body only stores, copies, or returns values.
Otherwise express the body-derived contract:

1. For numeric operations, use `slang-numeric-generics` when available.
2. For a domain-specific contract, declare a small interface containing exactly the required operations.
3. Match static versus instance members, receiver mutation, default construction, associated types, associated constants, and read-versus-write access.
4. Constrain the type on which the operations are actually invoked.
   Constraining a wrapper's element does not automatically make the wrapper conform.
5. Validate the proposed contract in a small probe before restructuring a large shader.

When the generic explicitly constructs `vector<T, N>`, `matrix<T, R, C>`, or another builtin shape, use the public `IBuiltinScalar...` constraint matching the element capability; do not expose implementation-level `__Builtin...` markers in the port.
When the abstracted type itself is passed to `dot`, consider `IDotProduct`, which covers built-in numeric scalars and ordinary vectors and returns the logical scalar type.
These interfaces require `import slang.numerics;` and a compiler invocation with `-experimental-feature`.
If the task's compiler or standard modules do not provide them, report that compatibility boundary instead of silently substituting legacy constraints.

Read [references/contracts-and-extensions.md](references/contracts-and-extensions.md) when declaring interfaces, conformances, or constrained extensions.

Every generic parameter on an extension should be determined by the extended type.
When an additional type or value parameter is used only by one operation, declare it on that member.
The member can constrain its new parameter in terms of the extension's parameters, but it cannot add a new constraint to an existing extension parameter; use a free generic function when the natural contract requires both parameters to be constrained together.

## Preserve finite value-dependent overloads

A generic body is checked while a value parameter such as `N` is abstract.
A dependent `T[N]` therefore does not select overloads that separately accept only `T[1]`, `T[2]`, or `T[3]`.
Neither `static_assert` nor an ordinary `if (N == K)` refines `T[N]` to `T[K]` for overload resolution.

Use this decision order:

1. If the per-value bodies are instances of one algorithm, make the callee value-generic and express that algorithm directly with `N`.
   Preserve the original finite domain with `static_assert` or an equivalent caller constraint.
2. If the per-value bodies are materially different, retain them behind a dispatcher type whose conformance is specialized for each supported value.
   Constrain the generic caller on that conformance so unsupported values remain ill-formed.
3. If neither formulation is expressible without changing semantics, report the minimized language limitation.

Read [references/finite-value-dispatch.md](references/finite-value-dispatch.md) for complete patterns and a counterexample that does not refine a dependent type.

Do not expect a scalar-to-`vector<T, 1>` conversion to lift through an array or another generic wrapper.
For example, `T[N]` and `vector<T, 1>[N]` remain different parameter types.
Preserve the declared shape with an exact overload or an explicit element-wise adapter; if the value is passed by `inout` or aliases storage, confirm that copying and writing back preserves the source behavior.

## Validate behavior, not just compilation

Run the supplied Slang compiler after each coherent change.
At completion, verify every requested entry point and emitted artifact, confirm that active HLSL template syntax is gone, and confirm that native generic abstractions remain.

Audit all declared value cases and representative type instantiations when probes are available.
Never replace a body with a default result, bypass a wave or resource operation, discard a write, or narrow to a visible specialization to obtain a compile.
If a faithful port remains blocked, preserve the best coherent port, record the blocker, and fail honestly.
