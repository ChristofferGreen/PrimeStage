# PrimeStage Example App Consumer Checklist

## Goal

Example apps should model canonical PrimeStage consumption patterns for application developers.

## Canonical Rules

1. Prefer widget-level PrimeStage specs/callbacks over direct `PrimeFrame::Callback` mutation.
2. Route host input through `PrimeStage::bridgeHostInputEvent(...)`.
3. Use `PrimeStage::FrameLifecycle` for rebuild/layout/frame scheduling.
4. Use `PrimeStage::WidgetIdentityReconciler` for focus restoration across rebuilds.
5. Avoid raw numeric key codes in example app logic; use `PrimeStage::KeyCode`/`HostKey`.
6. Avoid app-level `node->callbacks = ...` and ad-hoc callback composition helpers.

## Advanced Exceptions

Direct PrimeFrame usage is allowed in examples only when the operation is explicitly marked as an
advanced host/runtime integration concern and no PrimeStage wrapper currently exists.

Current documented advanced exceptions:

- Frame root creation and root-node sizing/layout bootstrap.
- Layout/focus/event-router orchestration at host-loop boundaries.

These exceptions must be clearly tagged inline in the example source with
`Advanced PrimeFrame integration (documented exception):`.

## Review Checklist

- Confirm canonical rules above are followed.
- Confirm any direct PrimeFrame internals are covered by the advanced-exception tag.
- Confirm simple examples (for example `examples/primestage_example.cpp`) avoid direct
  `PrimeFrame` headers and internals.
- Confirm new example logic is covered by unit guard checks in `tests/unit/test_api_ergonomics.cpp`.
