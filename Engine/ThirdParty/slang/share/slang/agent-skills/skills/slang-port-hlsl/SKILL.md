---
name: slang-port-hlsl
description: Port HLSL source to native Slang while preserving shader behavior, resource and memory semantics, and entry-point contracts. Use when Slang compilation exposes dialect, declaration, mutation, indexing, constructor, or resource differences that are not specific to template syntax.
license: Apache-2.0 WITH LLVM-exception
---

# Port HLSL to Slang

Produce native Slang code that preserves the source algorithm, resource behavior, layouts, and entry-point contracts.
Treat successful compilation as necessary evidence, not sufficient evidence of fidelity.

## Establish the baseline

Use the compiler commands and language modes supplied by the task.
Keep failures in the HLSL baseline, Slang port, and surrounding build or capture infrastructure distinct.
If the first failure comes from missing input, a malformed capture, or a broken command, report it before changing source.

Compile the port explicitly as Slang.
A `.hlsl` or `.usf` filename can otherwise select HLSL compatibility behavior.
Minimize the earliest unexpected diagnostic before undertaking a broad rewrite, and note whether fixing it removes later errors.

## Apply compatibility changes narrowly

- Remove a redundant C- or C++-style struct forward declaration only after confirming that the full definition is present.
- Move an unsupported out-of-line member body into the type or a suitable extension without changing visibility or generic constraints.
- Mark a receiver-mutating method `[mutating]`, including its interface requirement.
- Port HLSL `operator[]` declarations to Slang `__subscript` syntax while preserving read-only versus writable access.
- Preserve `inout` writeback.
  If a swizzle or property cannot be passed directly, use a local value and explicitly assign the modified result back.
- Preserve element-wise casts and matrix element order.
  Do not replace a vector conversion with component-zero extraction and splatting.
- Preserve memory and synchronization semantics, including `globallycoherent`, barriers, wave operations, and thread-group contracts.
- Check the binding and lifetime behavior of resource-bearing `static const` aggregates before translating them into Slang constants.

Keep general compatibility changes separate from template-to-generic changes in the porting log.
Read [references/common-differences.md](references/common-differences.md) when one of these language differences is involved.

## Stop before weakening behavior

Do not replace a body with a default value, bypass a wave or resource operation, discard a write, remove a synchronization or memory qualifier, or narrow the supported specialization domain merely to make the program compile.
If a remaining blocker cannot be solved faithfully, leave the best coherent port in place, document the smallest blocker and the evidence collected, and fail honestly.

Before finishing, audit consequential changes to:

- return types and returned values;
- resource reads, writes, bindings, qualifiers, and layouts;
- `inout` and receiver mutation;
- wave operations, barriers, thread-group sizes, and wave-size contracts;
- casts, component selection, and scalar-versus-shaped behavior;
- entry points and emitted artifacts.

Run every supplied verification command and preserve any required append-only attempt log.
