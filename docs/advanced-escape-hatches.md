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
- When directly touching PrimeFrame/PrimeHost internals in advanced sample code, tag the local
  integration site inline with:
  `Advanced PrimeFrame integration (documented exception):`
- When manually orchestrating lifecycle scheduling (`requestRebuild`, `requestLayout`,
  `requestFrame`) in advanced sample code, tag the local site inline with:
  `Advanced lifecycle orchestration (documented exception):`
- Review exception-tag policy details in `docs/example-app-consumer-checklist.md` before adding
  or changing advanced integrations.
- Review the full advanced integration checklist in
  `docs/example-app-consumer-checklist.md` before introducing new escape hatches.
- Review canonical-vs-advanced API guardrails in `docs/api-ergonomics-guidelines.md` before
  expanding advanced usage.
- Review default interaction/visual behavior expectations in
  `docs/default-widget-behavior-matrix.md` before changing advanced visual behavior.
- Review widget-spec default/advanced field classifications in
  `docs/widget-spec-defaults-audit.md` before introducing advanced per-widget overrides.
- Review high-level symbol contracts in `docs/minimal-api-reference.md` before exposing new
  advanced escape-hatch API paths.
- Review API compatibility and deprecation rules in `docs/api-evolution-policy.md` before adding
  advanced public-facing API surface.
- Review render failure reporting expectations in `docs/render-diagnostics.md` before adding
  advanced rendering escape hatches.
- Review callback reentrancy/threading guarantees in `docs/callback-reentrancy-threading.md`
  before composing advanced callback escape hatches.
- Review ownership and lifetime rules in `docs/data-ownership-lifetime.md` before composing
  advanced callback/interop escape hatches that capture borrowed data.
- Review dependency pinning/reproducibility policy in `docs/dependency-resolution-policy.md`
  before adding advanced integration examples that introduce external dependencies.
- Review visual snapshot/update workflow in `docs/visual-test-harness.md` before adding advanced
  interaction or rendering escape hatches.
- Review complexity/ergonomics budget expectations in `docs/api-ergonomics-scorecard.md` before
  expanding advanced-path complexity.
- Review contributor policy guardrails in `AGENTS.md` before introducing new advanced integration
  paths.
- Treat escape-hatch additions as docs+tests-coupled changes: update docs and
  `tests/unit/test_api_ergonomics.cpp` in the same change.
- When adding a new escape hatch, update:
  - `docs/example-app-consumer-checklist.md`
  - `docs/api-ergonomics-guidelines.md`
  - `AGENTS.md`
  - `tests/unit/test_api_ergonomics.cpp`
