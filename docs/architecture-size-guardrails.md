# PrimeStage Architecture Size Guardrails

## Goal

Keep implementation hotspots incrementally maintainable by failing CI when key files or functions
grow beyond agreed limits.

## Thresholds

### File-Size Thresholds

| File | Max Lines |
| --- | --- |
| `src/PrimeStageTreeView.cpp` | `1400` |
| `src/PrimeStageTextField.cpp` | `1000` |
| `src/PrimeStageSelectableText.cpp` | `760` |
| `src/PrimeStageTable.cpp` | `560` |
| `src/PrimeStage.cpp` | `1800` |

### Function-Size Thresholds

| Function Signature | Max Lines |
| --- | --- |
| `UiNode UiNode::createTreeView(TreeViewSpec const& spec)` | `1100` |
| `UiNode UiNode::createTextField(TextFieldSpec const& specInput)` | `900` |
| `UiNode UiNode::createSelectableText(SelectableTextSpec const& specInput)` | `650` |
| `UiNode UiNode::createTable(TableSpec const& specInput)` | `520` |

## CI Enforcement

- `scripts/lint_architecture_size.sh` enforces all thresholds.
- `.github/workflows/presubmit.yml` runs the script in the `Architecture Size Guardrails` job.
- CI must fail when any threshold is exceeded.

## Exception Policy

- Prefer refactoring/splitting before increasing limits.
- If a threshold increase is technically necessary, update this document and
  `scripts/lint_architecture_size.sh` in the same change with a short rationale in the PR.
- Threshold increases should be minimal and follow-up refactor work should be tracked in
  `docs/todo.md`.
