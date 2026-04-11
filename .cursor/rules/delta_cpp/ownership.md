---
paths:
  - "Engine/**/*.cpp"
  - "Engine/**/*.h"
---
# Object Ownership and Pointer Conventions

> This file **overrides** [cpp/coding-style.md](../cpp/coding-style.md) and [cpp/security.md](../cpp/security.md) smart pointer rules for `DObject`-derived types. Raw pointers are correct for reflected object relationships in DeltaEngine.

## `DObject`-derived relationships use raw pointers

Scene references, component back-pointers, and any runtime `DObject` relationships are **raw pointers**. Do not use `shared_ptr` or `unique_ptr` for these links.

```cpp
// Correct
DObject*        m_owner   = nullptr;
SceneComponent* m_parent  = nullptr;

// Wrong
std::shared_ptr<DObject> m_owner;
```

## Subsystem ownership

Engine subsystems owned exclusively by a single parent use `std::unique_ptr`.

## No raw `new` / `delete`

Never use `new` or `delete` directly for reflected engine objects.

```cpp
// Correct
auto* obj = CreateDObject<MyClass>();

// Wrong
auto* obj = new MyClass();
```
