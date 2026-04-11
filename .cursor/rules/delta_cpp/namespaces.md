---
paths:
  - "Engine/**/*.cpp"
  - "Engine/**/*.h"
---
# Namespaces

> This file **extends** [cpp/coding-style.md](../cpp/coding-style.md) with DeltaEngine namespace conventions.

## In `.cpp` files

Always open with `using namespace DeltaEngine;` at the top, after includes.
Do **not** prefix symbols with `DeltaEngine::` inside function definitions unless a genuine name conflict requires disambiguation.

```cpp
#include "Runtime/Core/DObject.h"

using namespace DeltaEngine;

void MySystem::Init()
{
    DObject* obj = ...;             // no DeltaEngine:: prefix needed
}
```

## In `.h` files

**Never** place `using namespace` at file scope in a header. No exceptions.

Since all project headers are wrapped in `DELTA_ENGINE_NS_BEGIN` / `DELTA_ENGINE_NS_END`, types declared within them are already inside the `DeltaEngine` namespace. No `DeltaEngine::` prefix is needed when referencing other engine types from within a project header.

```cpp
DELTA_ENGINE_NS_BEGIN

class MyComponent : public DComponent
{
    DObject*        m_owner  = nullptr;   // no DeltaEngine:: needed
    SceneComponent* m_parent = nullptr;
};

DELTA_ENGINE_NS_END
```

## Engine namespace guards

Wrap all DeltaEngine definitions with:

```cpp
DELTA_ENGINE_NS_BEGIN
// ...
DELTA_ENGINE_NS_END
```
