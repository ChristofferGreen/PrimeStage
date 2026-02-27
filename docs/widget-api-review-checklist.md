# PrimeStage Widget API Review Checklist

## Goal
Ensure every new widget/control API lands with modern, high-quality ergonomics by default.

This checklist is required for pull requests that add or materially change a public widget/control
API in `include/PrimeStage/Ui.h` (or equivalent app-facing API surface).

## Required Review Items

### 1. Default Readability
- Widget is visually readable with untouched default theme values.
- App code is not required to provide custom colors/style tokens for baseline usability.
- Focus/pressed/selection/disabled visuals remain distinguishable using defaults.

### 2. Minimal Constructor Path
- A canonical minimal usage path exists and is documented in 1-3 lines.
- `docs/widget-spec-defaults-audit.md` is updated for changed/added spec fields and classification.
- Default usage does not require a large `Spec` setup just to render/interact.
- Any required non-default fields are justified and documented.

### 3. Optional Callback Surface
- Baseline interaction works without callbacks when binding/state is supplied.
- Callbacks are semantic (`onActivate`, `onChange`, `onOpen`, `onSelect`) and optional.
- Callback-only interaction gating is not introduced for standard workflows.

### 4. State And Binding Story
- Controlled behavior is documented (value comes from spec fields).
- Binding-backed behavior is documented when applicable (`State<T>`, `Binding<T>`, `bind(...)`).
- Owned-default or state-backed behavior is documented where relevant (for example text widgets).
- Rebuild persistence/lifetime expectations are explicit.

### 5. Regression Coverage And Docs
- Tests added/updated for positive interaction path plus at least one diagnostic/negative case.
- `tests/unit/test_api_ergonomics.cpp` guardrails updated when API/doc contracts change.
- Related docs are updated in the same PR (`docs/minimal-api-reference.md`,
  `docs/api-ergonomics-guidelines.md`, and design/checklist docs as needed).
- Canonical examples remain high-level API consumers (`docs/example-app-consumer-checklist.md`).

## PR Gating
- Pull requests must complete the widget API checklist section in
  `.github/pull_request_template.md`.
- Reviewers should block merge when any required item above is unchecked or unsupported by tests.
