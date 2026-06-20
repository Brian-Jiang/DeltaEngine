# Phase 10 — Adopt: dynamic multicast on `TestComponent` (end-to-end validation)

**Goal:** Validate the full dynamic-delegate stack (codegen, reflection, serialization, broadcast) on an existing test-only reflected type. This is **not** a production gameplay API — it proves the Phase 8–9 machinery works before any real class adopts dynamic delegates.

**Depends on:** Phase 9.

---

## Why `TestComponent` (not `DObject`)

- `TestComponent` (`Engine/Runtime/Test/TestComponent.h`) is already reflected, has `DFUNCTION()` methods (`TestAdd`, `TestMultiply`, `TestFunction`), and is used across reflection/serialization/editor tests.
- Keeps zero per-instance delegate overhead on every live `DObject` (materials, textures, shaders, etc.).
- Matches UE5’s pattern: destruction/notification delegates are opt-in per class, not on the object base.

---

## Files

### Runtime

- Edit `Engine/Runtime/Test/TestComponent.h`
  - After Phase 9 macros exist, declare a **dynamic multicast** delegate with a simple signature for testing, e.g. one `int` parameter:
    ```cpp
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTestComponentEvent, int, Value);
    ```
  - Add a reflected property:
    ```cpp
    DPROPERTY()
    FTestComponentEvent OnTestEvent;
    ```
  - Add a small `DFUNCTION()` helper used as a broadcast target in tests:
    ```cpp
    DFUNCTION()
    void OnTestEventReceived(int value);
    ```
  - Optionally add `BroadcastTestEvent(int value)` as a `DFUNCTION(ShowAsButton)` or plain method that calls `OnTestEvent.Broadcast(value)` — keeps tests from needing friend access.

- Edit `Engine/Runtime/Test/TestComponent.cpp`
  - Implement `OnTestEventReceived` — set a **test-visible static** (e.g. `s_lastReceivedValue`) or increment `s_receiveCount` so GTest can assert without mocking.
  - Implement `BroadcastTestEvent` if added.

### Tests

- Create `Engine/Tests/Engine/Core/Delegates/DynamicDelegateTestComponentTests.cpp`
  - **Bind + broadcast:** Create two `TestComponent` instances (A listener, B emitter). Bind A’s `OnTestEventReceived` to B’s `OnTestEvent` via the dynamic binding API from Phase 8/9. Call `B->BroadcastTestEvent(42)`. Assert A’s static counter/value updated.
  - **Multi-subscriber:** Bind two handlers on the same delegate; one broadcast fires both.
  - **Serialize round-trip:** Serialize a `TestComponent` (or asset containing one) with an active binding `(ScriptPointer → listener, functionName = "OnTestEventReceived")`, deserialize, resolve pointers, broadcast again — handler still fires.
  - **Stale target:** Destroy the listener object via GC, broadcast from emitter — binding must not crash (skip or compact stale dynamic binding).

- Register the new test file in `Engine/Tests/Engine/CMakeLists.txt`.

### Codegen

- Ensure `TestComponent` is already in the DeltaHeaderTool manifest (it should be). A full build regenerates `TestComponent.generated.h/.cpp` with the new delegate type + property registration.

---

## Build & verify

```bat
Tools\Scripts\build-x64-debug-engine-tests.bat --automatic
Tools\Scripts\run-x64-debug-engine-tests.bat --automatic --gtest_filter=DynamicDelegateTestComponent*
Tools\Scripts\test-delta-header-tool.bat --automatic
```

---

## Acceptance criteria

- [ ] `TestComponent` exposes reflected `OnTestEvent` (`FDynamicMulticastDelegate` / generated type) and compiles through normal codegen.
- [ ] Dynamic bind → broadcast invokes a reflected `DFUNCTION` on another `TestComponent` with correct argument marshaling.
- [ ] Serialize → deserialize → resolve → broadcast round-trip works.
- [ ] Destroying a bound listener does not cause UB on subsequent broadcast.
- [ ] Engine test target and DeltaHeaderTool pytest suite pass.
- [ ] **No** `OnDestroyed` (or any dynamic multicast) added to base `DObject`.

---

## Out of scope (follow-up, not Phase 10)

- Production `OnDestroyed` on `GameObject` / `SceneComponent`.
- Editor Details panel UI for binding dynamic delegates.
- Blueprint-style “assign in editor” UX for delegate properties.
