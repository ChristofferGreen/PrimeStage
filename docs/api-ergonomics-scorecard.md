# PrimeStage API Ergonomics Scorecard

## Goal

Keep canonical PrimeStage usage compact, readable, and default-first while keeping advanced
samples maintainable.
This scorecard defines measurable ergonomics budgets that must stay green in CI.

## Metrics And Thresholds

| Metric | Measurement Scope | Threshold |
| --- | --- | --- |
| Canonical UI LOC (modern) | `examples/canonical/primestage_modern_api.cpp` `buildUi(...)` non-empty, non-comment lines | `<= 70` |
| Advanced UI LOC (widgets) | `examples/advanced/primestage_widgets.cpp` `rebuildUi(...)` non-empty, non-comment lines | `<= 220` |
| Average lines per widget instantiation (modern) | `buildUi(...)` non-empty, non-comment lines / widget-instantiation call count | `<= 6.0` |
| Average lines per widget instantiation (advanced widgets) | `rebuildUi(...)` non-empty, non-comment lines / widget-instantiation call count | `<= 6.0` |
| Minimum widget-instantiation coverage (modern) | Widget-instantiation call count in `buildUi(...)` | `>= 10` |
| Minimum widget-instantiation coverage (advanced widgets) | Widget-instantiation call count in `rebuildUi(...)` | `>= 35` |
| Required spec fields per standard widget | Default `*Spec{}` instantiation path for standard widgets (`Button`, `TextField`, `SelectableText`, `Toggle`, `Checkbox`, `Slider`, `ProgressBar`, `Tabs`, `Dropdown`, `List`, `Table`, `TreeView`, `ScrollView`, `Window`) | `0` |

## CI Enforcement

- Source scorecard thresholds are enforced by:
  - `tests/unit/test_api_ergonomics.cpp`
  - `tests/unit/test_builder_api.cpp`
- Presubmit/CI must fail if any metric crosses threshold.
- If thresholds need to change, update this file and the enforcing tests in the same change.
