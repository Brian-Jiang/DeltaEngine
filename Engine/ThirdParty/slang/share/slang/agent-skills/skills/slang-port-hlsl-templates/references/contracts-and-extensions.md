# Contracts and Constrained Extensions

Infer a type parameter's contract from operations in the template body, not only from concrete arguments visible at current call sites.
Keep the interface no broader than the body requires.

## Constraint placement

Constrain the type on which an operation is performed.
If a generic adds two `Pair<T>` values, constraining `T` does not define addition for `Pair<T>`.
Either make the body operate on `T` components or make `Pair<T>` conform to the interface required by the generic operation.

Use a `where` clause when the constrained type is constructed or when it makes the relationship clearer:

```slang
T readFirst<T, C>(C values)
    where C : IReadable<T>
{
    return values.read(0);
}
```

## User-defined contracts

Match the source operation precisely:

```slang
interface IReadable<T>
{
    T read(uint index);
}

interface IAccumulator<T>
{
    [mutating] void add(T value);
}
```

Do not put writable value arrays and reference-backed writable resources behind one interface unless their mutation semantics genuinely match.
Prefer a read-only contract when the body only reads.

## Constrained extensions

Every generic parameter declared by an extension should be determined by the extended type.
If a type parameter appears only in one member signature or constraint, declare it on that member instead of on the extension.

For example, `Source` belongs on `convertFrom` because it is not part of `Box<Destination>`:

```slang
interface IConvertTo<Destination>
{
    Destination convert();
}

struct Box<T>
{
    T value;
}

extension<Destination> Box<Destination>
{
    static Box<Destination> convertFrom<Source : IConvertTo<Destination>>(Box<Source> source)
    {
        return { source.value.convert() };
    }
}
```

Putting `Source` on the extension would leave it independent of the target type.
Slang can warn that such an extension is non-standard and may not make the member available as intended.

A member constraint can constrain its own `Source` parameter in terms of `Destination`, as above.
It cannot add a new constraint to the extension's existing `Destination` parameter.
If the natural contract is instead `Destination : IConvertFrom<Source>`, use a free generic function that owns both parameters, or redesign the contract without reversing its meaning merely to fit an extension.

Put generic parameters and their constraints before the extended type:

```slang
extension<T : IReadable<int>> T
{
    int first()
    {
        return this.read(0);
    }
}
```

For a generic wrapper, repeat the parameter relationship in the extension:

```slang
extension<T : IScalarAdditive> Pair<T> : IAdditive
{
    // Requirements use the wrapper as `This`.
}
```

See the compiler-checked [contract example](examples/contracts-and-extensions.slang).
