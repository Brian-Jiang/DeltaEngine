---
paths:
  - "Engine/**/*.cpp"
  - "Engine/**/*.h"
---
# General C++ Style

> This file **overrides** [cpp/coding-style.md](../cpp/coding-style.md) for DeltaEngine-specific conventions. Where they conflict, this file wins.

## Naming

| Element | Convention | Example |
|---|---|---|
| Types / classes | `PascalCase` | `MeshRenderer`, `DObject` |
| Methods | `PascalCase` | `GatherDrawCalls()` |
| Private / protected members | `m_camelCase` | `m_speed`, `m_ownerObject` |
| Local variables | `camelCase` | `localIndex`, `meshData` |
| Constants / enumerators | `PascalCase` or `UPPER_SNAKE` | `ELogLevel::Warning` |
| Files | `PascalCase` matching the primary type | `MeshRenderer.h/.cpp` |

## Braces and formatting

Opening brace on its own line (Allman style):

```cpp
void MyClass::DoThing()
{
    if (condition)
    {
        ...
    }
}
```

## Pointers

Always use `nullptr` over `NULL` or `0`.

## `const` correctness

- Mark all methods that do not mutate state `const`.
- Pass large objects by `const&`; pass primitives by value.
- Use `const` local variables wherever the value does not change after initialization.

## Forward declarations in headers

Prefer forward declarations over full includes in headers whenever possible:

```cpp
class DObject;
class SceneComponent;

class MySystem
{
    DObject*        m_root    = nullptr;
    SceneComponent* m_anchor  = nullptr;
};
```

## Compiler settings

- `/permissive-` is active — no non-standard extensions.
- `/WX` is applied to first-party targets only; never set globally via CMake.
- `/EHsc` is set globally.
- Third-party headers must be included as `SYSTEM` in CMake (`target_include_directories(... SYSTEM ...)`).
