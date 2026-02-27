# PrimeStage Data Ownership And Lifetime Contracts

## Scope

This document defines lifetime guarantees for public spec data (`std::string_view`, vectors, and callback captures).

## `std::string_view` In Widget Specs

PrimeStage spec string fields are non-owning inputs.

Contract:
- caller-provided `std::string_view` data must remain valid for the duration of the widget build call
- PrimeStage copies text into frame primitives where text must survive beyond the call
- callback payload data that can outlive build-call source buffers must use owned copies internally

Current guardrail:
- `Table` row callback payloads (`TableRowInfo::row`) are backed by PrimeStage-owned row strings, so
  `onRowClicked` does not depend on the original `TableSpec::rows` buffer lifetime.

## Callback Capture Ownership

PrimeStage copies callback functors from widget specs by value into callback tables.

Application contract:
- captures inside app callbacks must outlive callback use
- avoid capturing references to short-lived local variables
- prefer capturing owning values (`std::string`, `std::shared_ptr`, immutable structs) for deferred/rebuild-driven callbacks

## Rebuild And Lifetime Guidance

1. Treat spec objects as input descriptors, not persistent storage.
2. Keep backing storage alive through build, or pass owning temporary values that are copied immediately.
3. For callback data consumed after build (interaction callbacks), prefer owned state objects (`TextFieldState`, `ToggleState`, etc.).
4. For cross-thread state, synchronize externally before callback dispatch.

## Examples

Safe:

```cpp
std::string label = makeLabel();
PrimeStage::ButtonSpec spec;
spec.label = label;
root.createButton(spec); // label is alive during build
```

Risky:

```cpp
PrimeStage::ButtonSpec spec;
spec.label = std::string("tmp"); // temporary destroyed before use if stored as view
root.createButton(spec);
```

Safe callback capture:

```cpp
auto shared = std::make_shared<AppState>();
spec.callbacks.onClick = [shared]() { shared->count += 1; };
```
