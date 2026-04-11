---
paths:
  - "Engine/**/*.cpp"
  - "Engine/**/*.h"
---
# Reflection Macros

> This file **extends** [cpp/patterns.md](../cpp/patterns.md) with DeltaEngine's static reflection system conventions.

## Class annotation

```cpp
DELTA_ENGINE_NS_BEGIN

DCLASS()
class DELTAENGINE_API MyComponent : public DComponent
{
    DGENERATED_BODY(MyComponent)

public:
    ...

private:
    DPROPERTY()
    float m_speed = 0.0f;

    DPROPERTY()
    DObject* m_target = nullptr;
};

DELTA_ENGINE_NS_END
```

Rules:
- `DCLASS()` appears **immediately before** the `class` keyword, on its own line.
- `DGENERATED_BODY(ClassName)` is always the **first statement inside the class body**.
- `DPROPERTY()` appears **immediately before** the field it annotates, on its own line.
- `DFUNCTION()` appears **immediately before** the method it annotates, on its own line.
- Reflected classes must inherit (directly or transitively) from `DObject`.
- Do **not** reflect template classes or nested classes — the tool does not support them.
- `#include "ClassName.generated.h"` must appear at the **bottom** of the header (see include-order.md).

## Supported property types

| C++ type | Reflected as |
|---|---|
| `float` | `DFloatProperty` |
| `int` | `DIntProperty` |
| `bool` | `DBoolProperty` |
| `double` | `DDoubleProperty` |
| `std::string` | `DStringProperty` |
| `std::wstring` | `DWStringProperty` |
| `FVector3` / `DirectX::XMFLOAT3` | `DVector3Property` |
| `FQuaternion` | `DQuaternionProperty` |
| `DirectX::XMFLOAT4` | `DFloat4Property` |
| `DirectX::XMFLOAT4X4` | `DFloat4x4Property` |
| `T*` (DObject-derived) | `DObjectPtrProperty<T>` |
| `std::vector<T>` | `DVectorProperty<T>` |

Do not annotate fields whose types are not on this list with `DPROPERTY()`.

## Do not hand-edit generated files

Files under `Intermediate/DeltaHeaderTool/Generated/` are overwritten on every build.
