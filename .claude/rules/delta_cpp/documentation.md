---
paths:
  - "Engine/**/*.cpp"
  - "Engine/**/*.h"
---
# Documentation

> This file **extends** [cpp/coding-style.md](../cpp/coding-style.md) with DeltaEngine-specific documentation conventions.

Only document **public** fields and methods. Private and protected members do not need comments unless the logic is genuinely non-obvious.

Comments must be **simple, concise, and direct** — one line where possible. Do not restate what the signature already says.

```cpp
// Good — adds information the signature doesn't convey
/** Returns nullptr if the object has not been loaded yet. */
DELTAENGINE_API DObject* ResolveObject(ObjectId id) const;

/** Speed in units per second. */
DPROPERTY()
float m_speed = 0.0f;

// Bad — restates the obvious
/** Gets the name. */
DELTAENGINE_API std::string GetName() const;
```

Use `/** */` (Doxygen single-line style) for public API. Use `//` inline comments sparingly for non-obvious implementation detail in `.cpp` files.
