# PrimeStage Advanced Escape Hatches

## Goal

Document when to leave the canonical path and how to do it intentionally.
Default onboarding starts in `docs/5-minute-app.md`.

## Use Canonical First

Stay on the canonical tier unless you explicitly need low-level control:

- canonical examples: `examples/canonical/*.cpp`
- canonical app shell: `PrimeStage::App`
- canonical input path: `PrimeStage::App::bridgeHostInputEvent(...)`

If this covers your needs, do not drop to advanced APIs.

## Advanced Cases

### 1. Own the Host Event Loop

Use when you need custom frame pacing or host integration logic.

- reference: `examples/advanced/primestage_widgets.cpp`
- key APIs:
  - `PrimeHost::Host`
  - `PrimeHost::EventBuffer`
  - `PrimeStage::App::bridgeHostInputEvent(...)`
  - `PrimeStage::App::renderToTarget(...)`

### 2. Work Directly With Frame/Layout Primitives

Use when you need direct frame/layout output inspection or custom scene bootstrap.

- reference: `examples/advanced/primestage_scene.cpp`
- key APIs:
  - `PrimeFrame::Frame`
  - `PrimeFrame::LayoutEngine`
  - `PrimeFrame::LayoutOutput`

### 3. Compose Low-Level Node Callbacks

Use when high-level widget callbacks do not provide the behavior you need.

- key APIs under `PrimeStage::LowLevel`:
  - `LowLevel::NodeCallbackTable`
  - `LowLevel::NodeCallbackHandle`
  - `LowLevel::appendNodeOnEvent(...)`
  - `LowLevel::appendNodeOnFocus(...)`
  - `LowLevel::appendNodeOnBlur(...)`

### 4. Reach Raw Node IDs

Use only for explicit interop, not canonical app flow.

- escape hatch: `PrimeStage::UiNode::lowLevelNodeId()`

## Guardrails

- Keep advanced usage isolated to `examples/advanced` or explicitly advanced app modules.
- Do not leak low-level types into canonical examples or quick-start docs.
- When adding a new escape hatch, update:
  - `docs/example-app-consumer-checklist.md`
  - `docs/api-ergonomics-guidelines.md`
  - `tests/unit/test_api_ergonomics.cpp`
