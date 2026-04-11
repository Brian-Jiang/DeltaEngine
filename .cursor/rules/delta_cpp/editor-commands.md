---
paths:
  - "Engine/Editor/**/*.cpp"
  - "Engine/Editor/**/*.h"
---
# Editor Commands

> This file **extends** [cpp/patterns.md](../cpp/patterns.md) with DeltaEngine's editor command system conventions.

All property mutations and scene modifications initiated from the editor **must** go through the command system so they are undoable.

```cpp
// Correct — mutation through command
auto cmd = std::make_unique<EditorCommand_SetProperty>(...);
g_editorCore->GetCommandManager().Execute(std::move(cmd));

// Wrong — direct mutation bypasses undo stack
myObject->m_speed = newValue;
```

## Adding a new command

1. Subclass `EditorCommand` (undoable) or `EditorAuxiliaryCommand` (non-undoable).
2. Implement `Execute`, `Undo`, `Serialize`, `Deserialize`.
3. Declare `static constexpr std::string_view StaticTypeName()`.
4. Add a `static CommandRegistrar<T>` instance in the `.cpp` for auto-registration.
5. Commands identify objects by `AssetId` / `ObjectId` — never store raw pointers in a command.
