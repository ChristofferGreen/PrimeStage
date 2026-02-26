# Accessibility Semantics Roadmap

## Goal
Define a stable widget semantics model that can map PrimeStage widgets to future accessibility backends (for example HTML/ARIA or native platform accessibility trees).

## Scope
This roadmap defines metadata shape, behavior contracts, and rollout steps.
It does not yet add runtime accessibility tree emission.

## Metadata Model
PrimeStage exposes accessibility metadata in `include/PrimeStage/Ui.h` through:
- `AccessibilityRole`
- `AccessibilityState`
- `AccessibilitySemantics`

Relevant widget specs include `AccessibilitySemantics accessibility` so callers can describe role/state/label intent without touching backend internals.

### Role Mapping (Initial)
- `ButtonSpec` -> `AccessibilityRole::Button`
- `TextFieldSpec` -> `AccessibilityRole::TextField`
- `ToggleSpec` -> `AccessibilityRole::Toggle`
- `CheckboxSpec` -> `AccessibilityRole::Checkbox`
- `SliderSpec` -> `AccessibilityRole::Slider`
- `TabsSpec` -> `AccessibilityRole::TabList` (+ per-tab `Tab` in backend adapters)
- `DropdownSpec` -> `AccessibilityRole::ComboBox`
- `ProgressBarSpec` -> `AccessibilityRole::ProgressBar`
- `TableSpec` -> `AccessibilityRole::Table`
- `TreeViewSpec` -> `AccessibilityRole::Tree` (+ per-row `TreeItem` in backend adapters)
- Text widgets default to `StaticText` unless overridden.

## Focus Order Contract
Accessibility focus order should match PrimeStage keyboard focus order:
- primary source: widget-level `tabIndex` when present
- fallback source: deterministic layout/local-order traversal
- disabled widgets are excluded from focus traversal

This keeps keyboard and assistive-technology focus navigation aligned.

## Activation Contract
Keyboard activation semantics should remain consistent with accessibility activation expectations:
- `Enter`/`Space` activate button-like widgets
- selection controls (`Toggle`, `Checkbox`, `Tabs`, `Dropdown`) expose value/selection state changes through callbacks and state-backed models
- activation behavior must not require direct `PrimeFrame` callback mutation in app code

## State Semantics Contract
`AccessibilityState` should mirror user-visible widget state:
- `disabled`, `checked`, `selected`, `expanded`
- range/value fields for slider/progress-like controls (`valueNow`, `valueMin`, `valueMax`)
- hierarchical hints for tree-like widgets (`level`, `positionInSet`, `setSize`)

Backend adapters may derive missing fields from widget specs and runtime state when not supplied explicitly.

## Planned Runtime Integration
1. Add semantic extraction during scene build for supported widgets.
2. Add backend-neutral accessibility node representation (id, role, label, state, relations).
3. Provide adapter layers:
   - HTML/ARIA adapter
   - native host accessibility adapter(s)
4. Add snapshot/regression tests for semantic tree output and keyboard-focus parity.

## Test Coverage Strategy
Current coverage gates this roadmap by ensuring:
- public metadata types exist in `Ui.h`
- docs record focus-order and activation contracts
- keyboard focus/activation behavior remains regression-tested in interaction and focus suites
