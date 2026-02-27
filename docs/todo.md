# PrimeStage Implementation Plan (Prioritized)

Legend:
☐ Not started
◐ Started
☑ Complete

Priority:
- `P0` = blockers for API quality/correctness
- `P1` = important architecture/documentation alignment
- `P2` = lower-priority foundational cleanup

Release Exit Criteria (for API-quality milestone):
- no direct `PrimeFrame` node callback mutation required in app code for standard widgets
- no `PrimeFrame` include usage in canonical app examples (`examples/*.cpp`) outside explicitly-labeled low-level integration samples
- focus behavior is default-correct and visibly rendered for all interactive widgets
- keyboard and pointer behavior are consistent across widgets and platforms
- default theme is readable without app-side color or style-token configuration
- app code does not manually orchestrate rebuild/layout/frame scheduling for standard usage
- common widgets are usable with content/state only (callbacks optional, not required for baseline interaction)
- CI covers build/test matrix, interaction regressions, and visual state regressions
- docs match shipped API and canonical build/test workflow

## P0 (Do First)


## P1 (Do Next)

- ◐ [111] Split `src/PrimeStage.cpp` into focused internal implementation units.
  - extract per-widget build/interaction paths (`TextField`, `TreeView`, `Table`, `List`, etc.)
    into dedicated translation units to reduce blast radius and review complexity
  - preserve existing public API surface and behavior contracts while improving internal module boundaries
  - vertical slice shipped: moved `List` collection entrypoints into `src/PrimeStageCollections.cpp`
    with shared list-spec normalization routed through `PrimeStage::Internal::normalizeListSpec(...)`
  - vertical slice shipped: moved `ScrollView` entrypoints into `src/PrimeStageCollections.cpp`
    with shared scroll-spec normalization and node-construction seams routed via
    `PrimeStage::Internal` helpers
  - vertical slice shipped: moved `createTable(TableSpec)` runtime into
    `src/PrimeStageCollections.cpp` with shared table normalization, measurement, and
    focus/scrim helper seams routed via `PrimeStage::Internal`
  - vertical slice shipped: moved `createTreeView(TreeViewSpec)` runtime and tree-flatten/divider
    helpers into `src/PrimeStageCollections.cpp`, with shared tree normalization and text-node
    construction seams routed via `PrimeStage::Internal`
  - vertical slice shipped: moved `createProgressBar(ProgressBarSpec)` runtime into
    `src/PrimeStageProgress.cpp`, with shared progress-spec normalization, slider-event
    value mapping, and focus/scrim helper seams routed via `PrimeStage::Internal`
  - vertical slice shipped: moved `createDropdown(DropdownSpec)` runtime into
    `src/PrimeStageDropdown.cpp`, with shared dropdown-spec normalization and
    focus/scrim helper seams routed via `PrimeStage::Internal`
  - vertical slice shipped: moved `createTabs(TabsSpec)` runtime into
    `src/PrimeStageTabs.cpp`, with shared tabs-spec normalization and
    focus/scrim helper seams routed via `PrimeStage::Internal`
  - vertical slice shipped: moved `createToggle(ToggleSpec)` and
    `createCheckbox(CheckboxSpec)` runtime into `src/PrimeStageBooleanWidgets.cpp`,
    with shared toggle/checkbox normalization and focus/scrim helper seams routed
    via `PrimeStage::Internal`
  - vertical slice shipped: moved `createSlider(SliderSpec)` runtime into
    `src/PrimeStageSlider.cpp`, with shared slider-spec normalization and
    focus/scrim helper seams routed via `PrimeStage::Internal`
  - vertical slice shipped: moved `createButton(ButtonSpec)` runtime into
    `src/PrimeStageButton.cpp`, with shared button-spec normalization and
    focus/scrim helper seams routed via `PrimeStage::Internal`
  - vertical slice shipped: moved `createDivider(DividerSpec)` and
    `createSpacer(SpacerSpec)` runtime into `src/PrimeStageLayoutPrimitives.cpp`,
    with shared divider/spacer normalization routed via `PrimeStage::Internal`
  - vertical slice shipped: moved `createTextLine(TextLineSpec)` runtime into
    `src/PrimeStageTextLine.cpp`, with shared text-line normalization routed via
    `PrimeStage::Internal`
  - vertical slice shipped: moved `createLabel(LabelSpec)` runtime into
    `src/PrimeStageLabel.cpp`, with shared label normalization routed via
    `PrimeStage::Internal`
  - vertical slice shipped: moved `createParagraph(ParagraphSpec)` runtime into
    `src/PrimeStageParagraph.cpp`, with shared paragraph normalization routed
    via `PrimeStage::Internal`
  - vertical slice shipped: moved stack/container entrypoints (`createVerticalStack`,
    `createHorizontalStack`, `createOverlay`, `createPanel`) into
    `src/PrimeStageContainers.cpp`, with shared panel normalization routed via
    `PrimeStage::Internal`
  - vertical slice shipped: moved `createTextSelectionOverlay(...)` runtime into
    `src/PrimeStageTextSelectionOverlay.cpp`, with shared text-selection-overlay
    normalization routed via `PrimeStage::Internal`
  - vertical slice shipped: moved `createWindow(WindowSpec)` runtime into
    `src/PrimeStageWindow.cpp`, with shared window-spec normalization routed via
    `PrimeStage::Internal`
  - vertical slice shipped: moved `createSelectableText(SelectableTextSpec)` runtime into
    `src/PrimeStageSelectableText.cpp`, with shared selectable-text normalization and
    clamp/default helper seams routed via `PrimeStage::Internal`
  - vertical slice shipped: moved `createTextField(TextFieldSpec)` runtime into
    `src/PrimeStageTextField.cpp`, with shared text-field normalization/state/scrim
    helper seams routed via `PrimeStage::Internal`
  - vertical slice shipped: moved low-level callback composition and widget-identity
    reconciler runtime (`LowLevel::appendNodeOnEvent/...`, `NodeCallbackHandle`,
    `WidgetIdentityReconciler`) into `src/PrimeStageLowLevel.cpp`

- ☑ [119] Continue collection widget extraction from `src/PrimeStage.cpp`.
  - move `Table` build/interaction runtime into a dedicated collection translation unit
  - move `TreeView` build/interaction runtime into a dedicated tree translation unit
  - vertical slice shipped: moved `createTable(TableSpec)` and
    `createTable(columns, rows, selectedRow, size)` runtime into
    `src/PrimeStageTable.cpp`, keeping list/tree runtime in
    `src/PrimeStageCollections.cpp`
  - vertical slice shipped: moved `createTreeView(TreeViewSpec)` and
    `createTreeView(nodes, size)` runtime plus tree-specific helper seams
    into `src/PrimeStageTreeView.cpp`, keeping list/scroll runtime in
    `src/PrimeStageCollections.cpp`

- ☑ [120] Continue interactive widget extraction from `src/PrimeStage.cpp`.
  - move `TextField`/`SelectableText` build and interaction runtime into dedicated text translation units
  - extract shared text interaction patch helpers into focused internal modules
  - completed with dedicated text translation units
    (`src/PrimeStageTextField.cpp`, `src/PrimeStageSelectableText.cpp`) and
    shared text interaction helpers moved into
    `src/PrimeStageTextInteraction.cpp` (`measureTextWidth`, UTF-8 caret
    navigation helpers, selection helpers, and text-selection layout/caret-hit
    utilities)

- ◐ [112] Introduce an internal `WidgetRuntimeContext` shared runtime seam.
  - centralize shared patch/focus/callback helper state used across widget implementations
  - reduce duplicated runtime wiring and make interaction behavior easier to reason about/test
  - vertical slice shipped: added `Internal::WidgetRuntimeContext` + helper seams
    (`makeWidgetRuntimeContext`, `configureInteractiveRoot`,
    `appendNodeOnEvent`, context-scoped focus/scrim wrappers) in
    `src/PrimeStageCollectionInternals.h`/`src/PrimeStage.cpp`, and adopted the
    seam in `src/PrimeStageTable.cpp` and `src/PrimeStageWindow.cpp`
  - vertical slice shipped: adopted the runtime seam in
    `src/PrimeStageButton.cpp` and `src/PrimeStageBooleanWidgets.cpp`
    (`createButton`, `createToggle`, `createCheckbox`) so callback wiring,
    focusability setup, and focus/scrim overlays route through
    `WidgetRuntimeContext`
  - vertical slice shipped: adopted the runtime seam in
    `src/PrimeStageSlider.cpp`, `src/PrimeStageDropdown.cpp`, and
    `src/PrimeStageProgress.cpp` (`createSlider`, `createDropdown`,
    `createProgressBar`) so interaction roots, callback registration frame
    access, and focus/scrim overlays route through `WidgetRuntimeContext`
  - vertical slice shipped: adopted the runtime seam in
    `src/PrimeStageTabs.cpp` (`createTabs`) so tab-node interaction setup,
    callback registration frame access, and focus/scrim overlays route
    through `WidgetRuntimeContext`
  - vertical slice shipped: adopted the runtime seam in
    `src/PrimeStageSelectableText.cpp` (`createSelectableText`) so runtime
    frame access and focus/scrim overlays route through
    `WidgetRuntimeContext`
  - vertical slice shipped: adopted the runtime seam in
    `src/PrimeStageTextField.cpp` (`createTextField`) so runtime frame access
    and focus/scrim overlays route through `WidgetRuntimeContext`
  - vertical slice shipped: adopted the runtime seam in
    `src/PrimeStageCollections.cpp` (`createScrollView`) so parent/frame/return
    runtime wiring routes through `WidgetRuntimeContext`
  - vertical slice shipped: adopted the runtime seam in
    `src/PrimeStageTextLine.cpp`, `src/PrimeStageLabel.cpp`, and
    `src/PrimeStageParagraph.cpp` so text-widget parent/frame/return runtime
    wiring routes through `WidgetRuntimeContext`
  - vertical slice shipped: adopted the runtime seam in
    `src/PrimeStageContainers.cpp` and `src/PrimeStageLayoutPrimitives.cpp`
    so container/layout parent-frame-return runtime wiring routes through
    `WidgetRuntimeContext`
  - vertical slice shipped: adopted the runtime seam in
    `src/PrimeStageTextSelectionOverlay.cpp` (`createTextSelectionOverlay`)
    so text-selection overlay frame/parent runtime wiring routes through
    `WidgetRuntimeContext`
  - vertical slice shipped: adopted the runtime seam in
    `src/PrimeStageTreeView.cpp` (`createTreeView`) so tree parent/frame/return
    runtime wiring and focus/scrim overlays route through
    `WidgetRuntimeContext`
  - vertical slice shipped: adopted the runtime seam in
    `src/PrimeStageCollections.cpp` (`createList`) so list-entrypoint parent
    runtime wiring routes through `WidgetRuntimeContext`

- ☐ [113] Add CI architecture-size guardrails for implementation hotspots.
  - fail CI when agreed max thresholds are exceeded for single-file or single-function size
  - document thresholds and exception policy to keep refactors incremental and explicit

- ☐ [114] Add accessibility semantics snapshot/export regression coverage.
  - generate deterministic role/state snapshots for core widgets and verify expected semantics mappings
  - include contract coverage for default, focused, disabled, and selected states

- ☐ [115] Add migration notes/tests for semantic callback naming adoption.
  - document migration from legacy aliases (`onClick`, `onChanged`, etc.) to semantic callbacks
    (`onActivate`, `onChange`, etc.) with before/after examples
  - add regression tests ensuring both compatibility aliases and preferred callback paths remain correct


## P2 (Foundational Cleanup / Backlog)

- ☐ [116] Add deterministic host-input replay tests from recorded traces.
  - support replay fixtures for triaging interaction regressions with reproducible event streams
  - keep replay harness deterministic and versioned with repository test inputs

- ☐ [117] Expand property/fuzz coverage for widget-spec sanitization paths.
  - add randomized invalid-size/range/index inputs across widget specs and assert clamp invariants
  - keep seeds/corpus deterministic and include focused regression corpus for known failures

- ☐ [118] Add an internal extension seam for custom widget primitives.
  - provide a typed internal extension path for advanced composition without requiring direct
    `LowLevel` callback composition in standard integrations
  - document constraints so canonical API usage remains high-level and stable

## Archive (Completed)

Completed items moved here to keep active backlog focused.

### P1 (Do Next)

- ☑ [80] Align progress-bar zero-fill contract with patch-first interaction behavior.
  - reconciled zero-value progress-bar expectations in `tests/unit/test_widgets.cpp`:
    zero value with `minFillWidth == 0` now asserts hidden fill-node behavior
    (`visible=false`, width `0.0f`) instead of requiring node removal
  - added complementary positive coverage in the same test for
    `minFillWidth > 0` at zero value to confirm visible minimum-fill rendering
  - documented the contract in `docs/default-widget-behavior-matrix.md`
    (patch-first fill node remains allocated and hidden at zero value)
  - updated contributor guidance in `AGENTS.md` and enforcement checks in
    `tests/unit/test_api_ergonomics.cpp`

- ☑ [79] Clarify lifecycle scheduling exceptions between canonical and advanced examples.
  - aligned tier guardrails in `tests/unit/test_api_ergonomics.cpp` so canonical
    examples still forbid manual lifecycle orchestration while advanced examples
    may intentionally call `app.ui.lifecycle().requestRebuild()`
  - added explicit advanced-source tag in
    `examples/advanced/primestage_widgets.cpp`:
    `Advanced lifecycle orchestration (documented exception):`
  - documented lifecycle exception policy in
    `docs/example-app-consumer-checklist.md` and contributor guidance in
    `AGENTS.md`
  - ensures canonical constraints remain strict without falsely failing advanced
    integration examples that intentionally demonstrate orchestration behavior

- ☑ [77] Improve compile-time ergonomics and diagnostics.
  - added `constexpr` validation helpers in `include/PrimeStage/Ui.h` for
    common high-level API misuse:
    `bind(...)` state ownership shape, list/table/tree model extractor signatures,
    and model key extractor return type contracts
  - wired clearer static assertions for `makeListModel(...)`, `makeTableModel(...)`,
    and `makeTreeModel(...)` so failures explain expected extractor signatures and
    point directly to `docs/minimal-api-reference.md`
  - added compile-time regression assertions in
    `tests/unit/test_end_to_end_ergonomics.cpp` covering positive and negative
    validator outcomes for list/table/tree adapters and key extractors
  - expanded documentation/regression guardrails in
    `docs/minimal-api-reference.md`, `docs/api-ergonomics-guidelines.md`,
    `AGENTS.md`, and `tests/unit/test_api_ergonomics.cpp` so diagnostics guidance
    stays aligned with shipped API surfaces

- ☑ [76] Add structured widget-spec defaults audit.
  - added `docs/widget-spec-defaults-audit.md` with:
    - explicit classification rules (`required`, `optional`, `advanced`)
    - shared-base spec inventory (`SizeSpec`, `WidgetSpec`, `EnableableWidgetSpec`,
      `FocusableWidgetSpec`)
    - per-widget field inventory for all standard controls (`ButtonSpec`, `TextFieldSpec`,
      `SelectableTextSpec`, `ToggleSpec`, `CheckboxSpec`, `SliderSpec`, `TabsSpec`,
      `DropdownSpec`, `ProgressBarSpec`, `ListSpec`, `TableSpec`, `TreeViewSpec`,
      `ScrollViewSpec`, `WindowSpec`)
    - canonical minimal 1-line instantiation path for each standard widget
  - codified noisy-default policy in the audit doc (legacy callback aliases and deep style/state
    tuning classified as advanced, with default-first usage preferred)
  - linked audit doc from `README.md`, `docs/api-ergonomics-guidelines.md`,
    `docs/minimal-api-reference.md`, `docs/widget-api-review-checklist.md`, and `AGENTS.md`
  - expanded regression guardrails in `tests/unit/test_api_ergonomics.cpp` to enforce audit-doc
    presence/content, guideline/checklist/reference links, and alignment with default-fallback
    builder coverage

- ☑ [75] Add automated API surface linting.
  - added `scripts/lint_canonical_api_surface.sh` to detect forbidden low-level API surface usage
    in canonical example sources (`examples/canonical/*.cpp`) and canonical docs C++ snippets
    (`README.md`, `docs/5-minute-app.md`)
  - lint checks now fail on forbidden patterns (`PrimeFrame/*`, `PrimeFrame::`, raw `NodeId`,
    `lowLevelNodeId(...)`, and manual host-event translation markers such as
    `std::get_if<PrimeHost::...>`)
  - wired linting into CI presubmit with dedicated job `canonical-api-surface-lint` in
    `.github/workflows/presubmit.yml`
  - added regression/documentation guardrails in `tests/unit/test_api_ergonomics.cpp`,
    `README.md`, `docs/example-app-consumer-checklist.md`, and `AGENTS.md`

- ☑ [74] Add dedicated docs for "5-minute app" and "advanced escape hatches".
  - added dedicated onboarding docs:
    `docs/5-minute-app.md` (canonical-first quick start) and
    `docs/advanced-escape-hatches.md` (low-level/interop decision guide)
  - documented direct mapping from onboarding steps to canonical sample source
    (`examples/canonical/primestage_modern_api.cpp`) and explicit advanced references
    (`examples/advanced/primestage_widgets.cpp`, `examples/advanced/primestage_scene.cpp`)
  - updated documentation surfaces in `README.md`, `docs/api-ergonomics-guidelines.md`, and
    `AGENTS.md` to keep recommended and advanced usage paths explicitly separated
  - added regression guard coverage in `tests/unit/test_api_ergonomics.cpp` to enforce
    onboarding-doc presence, canonical/advanced content separation, and README/guideline links

- ☑ [73] Split examples into `canonical` and `advanced` tiers.
  - moved example sources into explicit tier directories:
    `examples/canonical/primestage_modern_api.cpp`,
    `examples/canonical/primestage_example.cpp`,
    `examples/advanced/primestage_widgets.cpp`, and
    `examples/advanced/primestage_scene.cpp`
  - rewired example targets in `CMakeLists.txt` and updated `README.md` first-use
    guidance to point to canonical tier before advanced samples
  - updated canonical-consumer/docs scorecard guidance in
    `docs/example-app-consumer-checklist.md`,
    `docs/api-ergonomics-scorecard.md`, and
    `docs/api-ergonomics-guidelines.md`
  - expanded tier guardrails in `tests/unit/test_api_ergonomics.cpp` to assert
    canonical low-level exclusions plus canonical/advanced source wiring in CMake/docs

### P0 (Do First)

- ☑ [110] Port compile-script coverage workflow from PrimeStruct.
  - added `--coverage` support to `scripts/compile.sh`:
    - configures clang coverage flags (`-fprofile-instr-generate -fcoverage-mapping`)
    - runs tests under `LLVM_PROFILE_FILE`, merges with `llvm-profdata`
    - emits text + HTML reports at
      `build-debug/coverage/coverage.txt` and `build-debug/coverage/html/`
  - added coverage regression checks in
    `tests/unit/test_api_ergonomics.cpp` to enforce compile-script flags/tooling
    wiring and AGENTS coverage documentation links
  - updated `AGENTS.md` build/test workflow with explicit `--coverage`
    usage and report output locations

- ☑ [109] Require canonical-first fallback text in checklist section.
  - updated `docs/advanced-escape-hatches.md` guardrails with explicit
    checklist-section phrasing that keeps canonical-first fallback visible
    (`if canonical tier coverage is sufficient, do not add a new escape hatch`)
  - expanded `tests/unit/test_api_ergonomics.cpp` to require that
    checklist-section canonical-first phrasing in advanced onboarding docs,
    preventing regressions where canonical fallback guidance only appears in
    earlier sections

- ☑ [108] Include advanced escape-hatch doc in its own update checklist.
  - updated `docs/advanced-escape-hatches.md` checklist so escape-hatch
    additions explicitly include updating `docs/advanced-escape-hatches.md`
    itself alongside related docs/tests
  - expanded `tests/unit/test_api_ergonomics.cpp` to require the
    `docs/advanced-escape-hatches.md` checklist entry in advanced onboarding
    docs, preventing omission of self-doc updates during new escape-hatch work

- ☑ [107] Require docs+tests coupling language for escape-hatch additions.
  - updated `docs/advanced-escape-hatches.md` guardrails with explicit
    “docs+tests-coupled changes” phrasing requiring docs updates and
    `tests/unit/test_api_ergonomics.cpp` updates in the same change
  - expanded `tests/unit/test_api_ergonomics.cpp` to require that coupling
    phrasing in advanced onboarding docs, preventing regression to checklist
    bullets without an explicit same-change coupling requirement

- ☑ [106] Link advanced onboarding docs to full checklist review policy.
  - updated `docs/advanced-escape-hatches.md` guardrails with explicit
    checklist-review phrasing requiring full review of
    `docs/example-app-consumer-checklist.md` before adding new escape hatches
    (not only exception-tag checks)
  - expanded `tests/unit/test_api_ergonomics.cpp` to require the full
    checklist-review phrasing in advanced onboarding docs, preventing
    regression to checklist coverage limited to exception tags

- ☑ [105] Link advanced onboarding docs to ergonomics scorecard policy.
  - updated `docs/advanced-escape-hatches.md` guardrails with an explicit
    handoff to `docs/api-ergonomics-scorecard.md` so advanced-path expansions
    review complexity/ergonomics budget expectations first
  - expanded `tests/unit/test_api_ergonomics.cpp` to require both the
    scorecard link and guardrail phrasing, preventing undocumented advanced
    complexity-budget drift

- ☑ [104] Link advanced onboarding docs to visual test harness policy.
  - updated `docs/advanced-escape-hatches.md` guardrails with an explicit
    handoff to `docs/visual-test-harness.md` so advanced interaction/rendering
    escape-hatch changes review visual snapshot/update workflow first
  - expanded `tests/unit/test_api_ergonomics.cpp` to require both the visual
    test harness link and guardrail phrasing, preventing undocumented advanced
    visual-regression workflow drift

- ☑ [103] Link advanced onboarding docs to dependency policy.
  - updated `docs/advanced-escape-hatches.md` guardrails with an explicit
    handoff to `docs/dependency-resolution-policy.md` so advanced integration
    examples review dependency pinning/reproducibility rules before adding new
    external dependencies
  - expanded `tests/unit/test_api_ergonomics.cpp` to require both the
    dependency policy link and guardrail phrasing, preventing undocumented
    advanced dependency-resolution drift

- ☑ [102] Link advanced onboarding docs to ownership/lifetime policy.
  - updated `docs/advanced-escape-hatches.md` guardrails with an explicit
    handoff to `docs/data-ownership-lifetime.md` so advanced callback/interop
    escape hatches review ownership/lifetime guarantees before capturing
    borrowed data
  - expanded `tests/unit/test_api_ergonomics.cpp` to require both the
    ownership/lifetime policy link and guardrail phrasing, preventing
    undocumented advanced lifetime-safety drift

- ☑ [101] Link advanced onboarding docs to callback threading policy.
  - updated `docs/advanced-escape-hatches.md` guardrails with an explicit
    handoff to `docs/callback-reentrancy-threading.md` so advanced callback
    escape-hatch composition reviews reentrancy/threading guarantees first
  - expanded `tests/unit/test_api_ergonomics.cpp` to require both the callback
    threading policy link and guardrail phrasing, preventing undocumented
    advanced callback-threading drift

- ☑ [100] Link advanced onboarding docs to render diagnostics policy.
  - updated `docs/advanced-escape-hatches.md` guardrails with an explicit
    handoff to `docs/render-diagnostics.md` so advanced rendering escape-hatch
    additions review render failure-reporting expectations first
  - expanded `tests/unit/test_api_ergonomics.cpp` to require both the
    render-diagnostics link and guardrail phrasing, preventing undocumented
    advanced render-path diagnostics drift

- ☑ [99] Link advanced onboarding docs to API evolution policy.
  - updated `docs/advanced-escape-hatches.md` guardrails with an explicit
    handoff to `docs/api-evolution-policy.md` so advanced public-facing API
    changes review compatibility/deprecation rules first
  - expanded `tests/unit/test_api_ergonomics.cpp` to require both the
    API evolution policy link and guardrail phrasing, preventing undocumented
    advanced API compatibility drift

- ☑ [98] Link advanced onboarding docs to minimal API reference policy.
  - updated `docs/advanced-escape-hatches.md` guardrails with an explicit
    handoff to `docs/minimal-api-reference.md` so advanced escape-hatch API
    additions review high-level symbol contracts first
  - expanded `tests/unit/test_api_ergonomics.cpp` to require both the
    minimal API reference link and guardrail phrasing, preventing undocumented
    advanced API-surface drift

- ☑ [97] Link advanced onboarding docs to widget-spec defaults policy.
  - updated `docs/advanced-escape-hatches.md` guardrails with an explicit
    handoff to `docs/widget-spec-defaults-audit.md` so advanced per-widget
    overrides review default/advanced field classification policy first
  - expanded `tests/unit/test_api_ergonomics.cpp` to require both the
    widget-spec defaults audit link and guardrail phrasing, preventing
    undocumented advanced override guidance drift

- ☑ [96] Link advanced onboarding docs to default-visual behavior policy.
  - updated `docs/advanced-escape-hatches.md` guardrails with an explicit
    handoff to `docs/default-widget-behavior-matrix.md` so advanced visual
    behavior changes review default interaction/visual expectations first
  - expanded `tests/unit/test_api_ergonomics.cpp` to require both the
    default-behavior matrix link and guardrail phrasing, preventing advanced
    visual changes from drifting away from documented defaults

- ☑ [95] Link advanced onboarding docs to API ergonomics policy guardrails.
  - updated `docs/advanced-escape-hatches.md` guardrails with an explicit
    pre-change handoff to `docs/api-ergonomics-guidelines.md` so advanced
    escape-hatch edits review canonical-vs-advanced policy guidance first
  - expanded `tests/unit/test_api_ergonomics.cpp` to require both the
    `docs/api-ergonomics-guidelines.md` reference and explicit policy-review
    phrasing in advanced onboarding docs, preventing silent drift to
    undocumented advanced-first guidance

- ☑ [94] Link advanced onboarding docs to contributor policy (`AGENTS.md`).
  - updated `docs/advanced-escape-hatches.md` guardrails with an explicit
    handoff to contributor policy in `AGENTS.md` for advanced integration
    changes
  - expanded `tests/unit/test_api_ergonomics.cpp` to require both `AGENTS.md`
    reference text and explicit contributor-policy review phrasing in advanced
    onboarding docs

- ☑ [93] Link advanced onboarding docs to exception-policy checklist.
  - updated `docs/advanced-escape-hatches.md` guardrails with an explicit
    exception-policy handoff to `docs/example-app-consumer-checklist.md`
  - expanded `tests/unit/test_api_ergonomics.cpp` to require that advanced
    onboarding guidance keeps both the checklist link and explicit policy-review
    phrasing, preventing drift between advanced docs and exception-tag policy

- ☑ [92] Keep advanced onboarding docs canonical-first (not default-path).
  - expanded `tests/unit/test_api_ergonomics.cpp` checks for
    `docs/advanced-escape-hatches.md` to enforce canonical-first framing:
    - must keep explicit handoff to `docs/5-minute-app.md`
    - must keep “stay on canonical tier unless needed” guidance
    - must keep “do not drop to advanced APIs” guidance
  - added negative guards so advanced onboarding doc does not drift into
    canonical quick-start/default-path wording (`Use This Path By Default` and
    5-minute app “build first app” phrasing)

- ☑ [91] Require advanced exception tags in advanced onboarding docs.
  - expanded `tests/unit/test_api_ergonomics.cpp` to require
    `docs/advanced-escape-hatches.md` to include both advanced exception
    markers:
    `Advanced PrimeFrame integration (documented exception):` and
    `Advanced lifecycle orchestration (documented exception):`
  - added regression checks that advanced onboarding doc references both
    `examples/advanced/primestage_widgets.cpp` and
    `examples/advanced/primestage_scene.cpp`
  - updated `docs/advanced-escape-hatches.md` guardrails with explicit
    instructions for both exception tags so advanced guidance remains complete
    while canonical docs stay tag-free

- ☑ [90] Keep advanced exception tags out of canonical docs surfaces.
  - expanded `tests/unit/test_api_ergonomics.cpp` to assert canonical-facing
    docs (`README.md` and `docs/5-minute-app.md`) do not contain advanced
    exception markers:
    `Advanced PrimeFrame integration (documented exception):` and
    `Advanced lifecycle orchestration (documented exception):`
  - added explicit `docs/5-minute-app.md` coverage checks for canonical-to-
    advanced mapping links while guarding against advanced exception-tag bleed
    into canonical onboarding content

- ☑ [89] Keep advanced exception tags out of canonical examples.
  - expanded `tests/unit/test_api_ergonomics.cpp` to assert canonical example
    sources do not contain advanced exception markers:
    `Advanced PrimeFrame integration (documented exception):` and
    `Advanced lifecycle orchestration (documented exception):`
  - added explicit canonical-tier checks for both
    `examples/canonical/primestage_modern_api.cpp` and
    `examples/canonical/primestage_example.cpp`

- ☑ [88] Enforce proximity of advanced PrimeHost runtime exception tags.
  - tightened `tests/unit/test_api_ergonomics.cpp` so advanced `PrimeHost::`
    runtime markers (not just includes) require a nearby inline exception tag
    `Advanced PrimeFrame integration (documented exception):`
  - added a positive guard proving PrimeHost runtime-marker coverage is
    exercised by advanced example sources
  - added local inline exception tags across PrimeHost runtime integration
    clusters in `examples/advanced/primestage_widgets.cpp` (surface bootstrap,
    callback wiring, event polling, and host-handle ownership)

- ☑ [87] Enforce proximity of advanced PrimeHost include exception tags.
  - tightened `tests/unit/test_api_ergonomics.cpp` to require advanced
    `#include "PrimeHost/..."` markers to have a nearby inline exception tag
    `Advanced PrimeFrame integration (documented exception):`
  - added positive include-marker coverage guards and checklist string checks
    so PrimeHost exception-policy wording is regression-tested
  - added an inline advanced integration tag adjacent to the PrimeHost include
    block in `examples/advanced/primestage_widgets.cpp`
  - clarified exception policy wording in
    `docs/example-app-consumer-checklist.md` and `AGENTS.md` so the existing
    advanced integration tag is explicitly documented for PrimeFrame/PrimeHost
    internals

- ☑ [86] Enforce proximity of advanced PrimeFrame include exception tags.
  - tightened `tests/unit/test_api_ergonomics.cpp` to require advanced
    `#include "PrimeFrame/..."` markers to have a nearby inline exception tag
    `Advanced PrimeFrame integration (documented exception):`
  - added a positive guard ensuring include-marker coverage is actually
    exercised by at least one advanced example source
  - moved the inline PrimeFrame exception tag in
    `examples/advanced/primestage_scene.cpp` to sit adjacent to the PrimeFrame
    include block so include-level proximity checks stay deterministic

- ☑ [85] Enforce proximity of advanced lifecycle exception tags.
  - tightened `tests/unit/test_api_ergonomics.cpp` so advanced-tier lifecycle
    scheduling calls (`requestRebuild` / `requestLayout` / `requestFrame`)
    require a nearby inline exception tag
    `Advanced lifecycle orchestration (documented exception):`
  - retained file-level lifecycle exception-tag enforcement while adding
    per-marker proximity assertions with line diagnostics for easier triage
  - added missing inline lifecycle exception tag near the explicit startup
    `requestFrame()` orchestration site in
    `examples/advanced/primestage_widgets.cpp`

- ☑ [84] Enforce proximity of advanced PrimeFrame exception tags.
  - tightened `tests/unit/test_api_ergonomics.cpp` so advanced-tier
    `PrimeFrame::` integration lines require a nearby inline exception tag
    `Advanced PrimeFrame integration (documented exception):` (line-proximity
    check with deterministic diagnostics)
  - retained file-level exception-tag enforcement while adding per-marker
    proximity assertions for stronger policy guarantees
  - added inline exception tags at remaining advanced PrimeFrame integration
    sites in `examples/advanced/primestage_widgets.cpp` and
    `examples/advanced/primestage_scene.cpp` so each integration cluster is
    locally documented

- ☑ [83] Enforce advanced PrimeFrame exception tagging across advanced examples.
  - expanded `tests/unit/test_api_ergonomics.cpp` to scan
    `examples/advanced/*.cpp` and enforce that any file with direct
    `PrimeFrame` integration (`#include "PrimeFrame/..."` or `PrimeFrame::`)
    includes the inline tag
    `Advanced PrimeFrame integration (documented exception):`
  - added a positive guard that at least one advanced example exercises direct
    PrimeFrame integration, with per-file diagnostics for easier triage
  - tagged `examples/advanced/primestage_scene.cpp` and
    `examples/advanced/primestage_widgets.cpp` inline at intentional direct
    PrimeFrame integration sites to satisfy the documented advanced-tier
    exception contract

- ☑ [82] Enforce advanced lifecycle exception tagging across all advanced examples.
  - expanded `tests/unit/test_api_ergonomics.cpp` to scan
    `examples/advanced/*.cpp` and enforce that any file calling
    `app.ui.lifecycle().requestRebuild()` / `requestLayout()` / `requestFrame()`
    includes the inline tag
    `Advanced lifecycle orchestration (documented exception):`
  - added deterministic traversal (`std::sort`) plus a positive guard that at
    least one advanced example exercises lifecycle orchestration
  - closes a policy-enforcement gap where only
    `examples/advanced/primestage_widgets.cpp` was checked for lifecycle
    exception tagging

- ☑ [81] Stabilize collection callback regression harness under layered callbacks.
  - updated `createTable(...)` pointer selection wiring in `src/PrimeStage.cpp`
    to install row-level pointer callbacks so row selection callbacks fire
    reliably under router dispatch (including short-lived row payload tests)
  - fixed callback-node discovery helpers in
    `tests/unit/test_interaction.cpp` and
    `tests/unit/test_spec_validation.cpp` to prefer descendant row callbacks
    before container-level callbacks, avoiding false selection failures when
    `createTable(...)` installs keyboard handlers on parent nodes
  - updated `tests/unit/test_spec_validation.cpp` to dispatch pointer input
    through layout/router instead of directly invoking callback internals
  - restored failing regression coverage for table/list pointer selection and
    row payload lifetime assertions in both interaction and spec-validation suites
  - hardened callback reentrancy API guard in
    `tests/unit/test_api_ergonomics.cpp` by checking split header comment
    fragments instead of one brittle line-wrapped literal

- ☑ [78] Fix `AppActionInvocation::actionId` lifetime safety.
  - `AppActionInvocation::actionId` in `include/PrimeStage/App.h` now uses owned
    `std::string` storage instead of `std::string_view` so callbacks can safely
    persist invocation payloads after `invokeAction(...)` returns
  - `src/App.cpp` now moves the copied action id into invocation payload storage
    before callback dispatch
  - added regression coverage in `tests/unit/test_app_runtime.cpp`:
    `App action invocation retains action id after callback lifetime ends`
    (covers callback-time action unregister + post-dispatch payload checks)
  - expanded docs/guardrails in `docs/minimal-api-reference.md`,
    `docs/api-ergonomics-guidelines.md`, `tests/unit/test_api_ergonomics.cpp`,
    and `AGENTS.md` to document and enforce the owned action-id contract

- ☑ [72] Add full end-to-end ergonomics regression tests.
  - added dedicated suite `tests/unit/test_end_to_end_ergonomics.cpp` covering
    canonical high-level `PrimeStage::App` flows across mouse, keyboard, and
    text input dispatch via `bridgeHostInputEvent(...)` with no
    `PrimeStage::LowLevel` hooks
  - added compile-time regression guards in the same suite
    (`SupportsDeclarativeConvenienceErgonomics<PrimeStage::UiNode>` and
    negative `std::is_invocable_v` misuse check) for convenience overload and
    declarative builder surfaces
  - wired suite into the test target in `CMakeLists.txt` and expanded CI
    fail-fast smoke filters in `.github/workflows/presubmit.yml` to include
    `*ergonomics*` in both primary and headless compatibility jobs
  - expanded source/workflow guardrails in `tests/unit/test_api_ergonomics.cpp`
    to require suite wiring, CI gating, and high-level-only suite authoring
  - documented suite ownership in `AGENTS.md` and linked it from
    `docs/api-ergonomics-guidelines.md`

- ☑ [71] Add API review checklist for any new widget/control.
  - added required review checklist doc
    `docs/widget-api-review-checklist.md` covering default readability, minimal
    constructor path, optional callbacks, state/binding ownership model, and
    test/doc gate requirements
  - added repository PR gate in `.github/pull_request_template.md` with a
    mandatory widget API checklist section for new/changed widgets
  - wired checklist references into contributor/docs surfaces in `AGENTS.md`,
    `docs/api-ergonomics-guidelines.md`, and `README.md`
  - expanded enforcement coverage in `tests/unit/test_api_ergonomics.cpp` to
    require checklist doc/template presence and required checklist fields

- ☑ [70] Introduce low-level API quarantine and naming.
  - quarantined advanced callback composition primitives under
    `PrimeStage::LowLevel` in `include/PrimeStage/Ui.h`
    (`LowLevel::NodeCallbackTable`, `LowLevel::NodeCallbackHandle`,
    `LowLevel::appendNodeOnEvent`, `LowLevel::appendNodeOnFocus`, and
    `LowLevel::appendNodeOnBlur`)
  - preserved compatibility through deprecated root-level aliases/wrappers so
    existing integrations keep compiling while new code migrates to
    `PrimeStage::LowLevel`
  - moved runtime implementation/call sites in `src/PrimeStage.cpp` to explicit
    `LowLevel::...` names so high-level internals do not rely on root escape
    hatches
  - expanded API/docs guardrails and runtime coverage in
    `tests/unit/test_api_ergonomics.cpp` to enforce the `namespace LowLevel`
    contract and canonical-example high-level-only usage
  - updated docs/guidance in `docs/callback-reentrancy-threading.md`,
    `docs/api-ergonomics-guidelines.md`, `docs/prime-stage-design.md`,
    `docs/minimal-api-reference.md`, `docs/example-app-consumer-checklist.md`,
    and `AGENTS.md`

- ☑ [69] Add migration path toward retained-state widgets with owned defaults.
  - added owned-default text-widget state slots in public specs:
    `TextFieldSpec::ownedState` and `SelectableTextSpec::ownedState` in
    `include/PrimeStage/Ui.h`, while preserving raw-pointer state-backed compatibility
  - updated `src/PrimeStage.cpp` text-widget construction to provision owned state when explicit
    state pointers are omitted, so `TextField`/`SelectableText` default instantiation remains
    interactive
  - expanded regression coverage in `tests/unit/test_api_ergonomics.cpp` for:
    state-backed compatibility, default-owned text field editing, owned-state rebuild persistence,
    selectable-text owned-default callback wiring, and docs/source contract guards
  - migrated canonical example usage in `examples/primestage_widgets.cpp` to owned-default text
    widget construction (no raw `TextFieldState` / `SelectableTextState` fields required)
  - updated docs/guidance in `docs/api-ergonomics-guidelines.md`,
    `docs/minimal-api-reference.md`, `docs/prime-stage-design.md`,
    `docs/default-widget-behavior-matrix.md`, and `AGENTS.md`

- ☑ [68] Harden default behavior consistency matrix.
  - added `docs/default-widget-behavior-matrix.md` with per-widget defaults for focusability,
    keyboard activation, pointer semantics, and accessibility role
  - aligned default runtime behavior for consistency:
    `ProgressBar` now supports default pointer/keyboard adjustment when enabled, and `Table`/`List`
    support keyboard row selection parity with pointer row selection
  - added centralized default accessibility-semantic role/state assignment in
    `src/PrimeStage.cpp` for standard interactive widgets and text/window defaults
  - expanded regression coverage in `tests/unit/test_interaction.cpp` for progress and table/list
    default interaction behavior plus docs/source contract guards in
    `tests/unit/test_api_ergonomics.cpp`
  - linked matrix policy into contributor/reference docs (`AGENTS.md`,
    `docs/api-ergonomics-guidelines.md`, and `docs/minimal-api-reference.md`)

- ☑ [67] Add explicit API ergonomics scorecard and measurable thresholds.
  - added `docs/api-ergonomics-scorecard.md` with explicit thresholds for canonical UI LOC,
    average lines per widget instantiation, widget-call coverage floors, and required spec-field
    assignments for standard widgets
  - enforced scorecard thresholds in CI/presubmit through
    `tests/unit/test_api_ergonomics.cpp` using source-derived metrics from
    `examples/primestage_modern_api.cpp` and `examples/primestage_widgets.cpp`
  - expanded default-spec fallback coverage in `tests/unit/test_builder_api.cpp` to verify
    standard widgets are baseline-instantiable with zero required spec-field assignments
  - linked scorecard policy into contributor and canonical-example docs
    (`AGENTS.md`, `docs/api-ergonomics-guidelines.md`, and
    `docs/example-app-consumer-checklist.md`)

- ☑ [66] Add built-in form API for common input workflows.
  - added `FormSpec` and `FormFieldSpec` plus declarative `UiNode` primitives
    `form(...)` and `formField(...)` in `include/PrimeStage/Ui.h` for label+control+help/error
    composition without ad-hoc row/stack boilerplate
  - migrated settings-like canonical example flows in `examples/primestage_widgets.cpp` to use the
    new form primitives for text input, selection notes, and release-channel controls
  - added builder/runtime regression coverage in `tests/unit/test_builder_api.cpp` for form
    composition and invalid-spacing diagnostics
  - expanded API/docs/example guardrails in `tests/unit/test_api_ergonomics.cpp` to enforce form
    API visibility and canonical example usage
  - updated docs (`docs/minimal-api-reference.md`, `docs/api-ergonomics-guidelines.md`,
    `docs/prime-stage-design.md`, and `docs/example-app-consumer-checklist.md`) to document and
    require the built-in form authoring path

- ☑ [65] Add standardized command/action routing.
  - added app-level action APIs in `PrimeStage::App`:
    `registerAction`, `unregisterAction`, `bindShortcut`, `unbindShortcut`, `invokeAction`,
    and `makeActionCallback`, plus typed routing metadata
    (`AppActionSource`, `AppShortcut`, `AppActionInvocation`)
  - wired shortcut dispatch into `App::bridgeHostInputEvent(...)` so key chords route through
    action ids before widget-level dispatch without per-widget raw key plumbing
  - updated `examples/primestage_widgets.cpp` to register actions, bind shortcuts, and reuse the
    same action ids from button callbacks (`makeActionCallback`) for unified command entrypoints
  - expanded runtime and regression coverage in `tests/unit/test_app_runtime.cpp` and
    `tests/unit/test_api_ergonomics.cpp` to validate action routing behavior, repeat-policy
    diagnostics, canonical example usage, and docs/header API visibility
  - updated docs (`docs/minimal-api-reference.md`, `docs/api-ergonomics-guidelines.md`,
    `docs/prime-stage-design.md`, `docs/example-app-consumer-checklist.md`) to document the
    standardized command-routing flow

- ☑ [64] Add ergonomic convenience overloads for common widgets.
  - added concise declarative `UiNode` helpers for common value widgets:
    `toggle(bind)`, `checkbox(label, bind)`, `slider(bind, vertical)`,
    `tabs(labels, bind)`, `dropdown(options, bind)`, and `progressBar(bind)`
  - updated `examples/primestage_widgets.cpp` to use concise overloads across major interactive
    controls instead of repetitive default `Spec` boilerplate
  - expanded regression coverage in `tests/unit/test_builder_api.cpp` for helper behavior and
    empty-options clamping diagnostics, and in `tests/unit/test_api_ergonomics.cpp` to enforce
    canonical helper usage plus docs/header discoverability
  - updated API docs (`docs/minimal-api-reference.md`, `docs/api-ergonomics-guidelines.md`,
    `docs/prime-stage-design.md`) to document the concise overload surface

- ☑ [63] Publish a strict "Modern API" canonical example and gate it in CI.
  - added `examples/primestage_modern_api.cpp` as a strict high-level canonical sample using only
    `PrimeStage::App` + `UiNode` surfaces (no PrimeFrame/PrimeHost internals or low-level escapes)
  - wired the example into build outputs in `CMakeLists.txt` as `primestage_modern_api`
  - updated `README.md` to make `primestage_modern_api` the default quick-start canonical example
  - expanded CI guardrails in `tests/unit/test_api_ergonomics.cpp` to enforce low-level API bans
    and defined complexity thresholds (line count, spec usage, creation-call count) for the modern
    example
  - updated checklist guidance in `docs/example-app-consumer-checklist.md` to keep the strict
    canonical example contract explicit

- ☑ [62] Replace full-rebuild app pattern with incremental invalidation defaults.
  - updated `PrimeStage::App` runtime scheduling defaults in `src/App.cpp` so
    `dispatchFrameEvent(...)`, `bridgeHostInputEvent(...)`, and handled imperative widget events
    automatically request frame presentation
  - removed canonical demo interaction-time lifecycle micromanagement from
    `examples/primestage_widgets.cpp` (no ordinary interaction callback
    `requestRebuild`/`requestLayout`/`requestFrame` calls)
  - expanded regression guardrails in `tests/unit/test_api_ergonomics.cpp` and
    `tests/unit/test_app_runtime.cpp` to enforce canonical no-spam lifecycle behavior and App
    auto-frame scheduling
  - updated docs in `docs/api-ergonomics-guidelines.md`,
    `docs/minimal-api-reference.md`, `docs/prime-stage-design.md`, and
    `docs/example-app-consumer-checklist.md` to document default incremental scheduling behavior

- ☑ [61] Add domain-model adapters for collection widgets.
  - added `ListModelAdapter`/`TableModelAdapter`/`TreeModelAdapter` and
    `makeListModel(...)`/`makeTableModel(...)`/`makeTreeModel(...)` in
    `include/PrimeStage/Ui.h`
  - adapters support typed model extraction for list rows, table rows/cells, and tree nodes with
    optional key extractors plus spec bind helpers (`bind`, `bindRows`, `bind`)
  - migrated `examples/primestage_widgets.cpp` collection sections to bind list/table/tree domain
    containers directly through adapters (no manual `std::vector<std::string_view>` conversion loops)
  - expanded regression coverage in `tests/unit/test_api_ergonomics.cpp` for adapter behavior,
    key extraction, and canonical example guardrails
  - documented adapter usage in `docs/minimal-api-reference.md`,
    `docs/api-ergonomics-guidelines.md`, `docs/prime-stage-design.md`, and
    `docs/example-app-consumer-checklist.md`

- ☑ [60] Introduce typed widget handles and remove `NodeId` from app-facing APIs.
  - added typed handles (`WidgetFocusHandle`, `WidgetVisibilityHandle`, `WidgetActionHandle`) to
    `include/PrimeStage/Ui.h` and exposed them from `UiNode` via
    `focusHandle()`/`visibilityHandle()`/`actionHandle()`
  - added app-facing typed-handle operations in `include/PrimeStage/App.h` / `src/App.cpp`
    (`focusWidget`, `isWidgetFocused`, `setWidgetVisible`, `setWidgetHitTestVisible`,
    `setWidgetSize`, `dispatchWidgetEvent`)
  - kept raw node ids as explicitly low-level via `lowLevelNodeId()` and tightened canonical
    example guardrails to ban `.nodeId(...)` usage in `examples/primestage_widgets.cpp`
  - expanded regression coverage in `tests/unit/test_app_runtime.cpp` and
    `tests/unit/test_api_ergonomics.cpp`
  - updated API/docs references in `docs/minimal-api-reference.md`,
    `docs/api-ergonomics-guidelines.md`, `docs/prime-stage-design.md`, and
    `docs/example-app-consumer-checklist.md`

- ☑ [59] Reduce mandatory manual sizing and layout micromanagement.
  - added intrinsic fallback sizing for unsized `ScrollView` and empty collection widgets
    (`Table`/`List`/`TreeView`) in `src/PrimeStage.cpp`
  - added responsive width policy wiring so `Paragraph` and `SelectableText` consume
    `size.maxWidth` when widget `maxWidth` is not set
  - simplified `examples/primestage_widgets.cpp` to rely on intrinsic defaults with only occasional
    explicit size hints
  - expanded regression coverage in `tests/unit/test_widgets.cpp` and tightened canonical-example
    size-hint guardrails in `tests/unit/test_api_ergonomics.cpp`
  - documented sizing policy updates in `docs/api-ergonomics-guidelines.md`,
    `docs/minimal-api-reference.md`, and `AGENTS.md`

- ☑ [58] Add readable semantic-default styling contract.
  - defined contract minimums (`4.5:1` text contrast, `3.0:1` focus contrast) in
    `docs/api-ergonomics-guidelines.md` and `docs/visual-test-harness.md`
  - hardened default-theme fallback selection in `src/PrimeStage.cpp` to enforce contrast
    thresholds for semantic text/focus colors
  - delivered default-theme readability visual regression coverage in
    `tests/unit/test_visual_regression.cpp` with golden snapshot
    `tests/snapshots/default_theme_readability.snap`

- ☑ [57] Provide high-level platform service integration.
  - delivered via `AppPlatformServices` plus `connectHostServices(...)` and
    `applyPlatformServices(...)` in `include/PrimeStage/App.h` / `src/App.cpp`, canonical
    text-widget migration in `examples/primestage_widgets.cpp` (no manual per-widget
    clipboard/cursor lambdas), and regression coverage updates in
    `tests/unit/test_app_runtime.cpp` and `tests/unit/test_api_ergonomics.cpp`

- ☑ [56] Define and enforce a minimal callback surface.
  - delivered via semantic callback fields in `include/PrimeStage/Ui.h`
    (`onActivate`/`onChange`/`onOpen`/`onSelect`) with legacy alias compatibility, runtime
    semantic-callback-first dispatch in `src/PrimeStage.cpp`, canonical example cleanup in
    `examples/primestage_widgets.cpp` (no callback plumbing required to enable baseline
    interactions for toggle/checkbox/tabs/dropdown), and regression updates in
    `tests/unit/test_interaction.cpp`, `tests/unit/test_tabs_dropdown.cpp`, and
    `tests/unit/test_api_ergonomics.cpp`

- ☑ [55] Add first-class state/binding primitives.
  - delivered via `State<T>`/`Binding<T>` + `bind(...)` in `include/PrimeStage/Ui.h`, binding-first
    interaction wiring for toggle/checkbox/slider/tabs/dropdown/progress in `src/PrimeStage.cpp`,
    canonical example migration in `examples/primestage_widgets.cpp`, and regression coverage
    updates in `tests/unit/test_interaction.cpp`, `tests/unit/test_tabs_dropdown.cpp`, and
    `tests/unit/test_api_ergonomics.cpp`

- ☑ [54] Add a declarative UI composition API.
  - delivered via declarative `UiNode` helpers in `include/PrimeStage/Ui.h` (`column`, `row`,
    `overlay`, `panel`, `label`, `paragraph`, `textLine`, `divider`, `spacer`, `button`,
    `window`), canonical gallery migration in `examples/primestage_widgets.cpp`, and regression
    coverage updates in `tests/unit/test_builder_api.cpp` + `tests/unit/test_api_ergonomics.cpp`

- ☑ [53] Ship a high-level `App` API that hides `PrimeFrame` internals.
  - delivered via `include/PrimeStage/App.h` + `src/App.cpp` (high-level app shell for rebuild/layout/router/focus/input/render lifecycle), canonical migration in `examples/primestage_widgets.cpp`, and regression coverage updates in `tests/unit/test_app_runtime.cpp`, `tests/unit/test_api_ergonomics.cpp`, and `tests/cmake/find_package_smoke/main.cpp`

- ☑ [52] Remove callback-gated interaction from stateful controls.
  - slider/progress remain interactive when state is provided, without requiring `onValueChanged`
  - `SliderSpec` now supports state-backed mode (`SliderState*`) for controlled/uncontrolled parity with other controls
  - delivered by slider state-backed interaction wiring in `src/PrimeStage.cpp`, API/docs updates in `include/PrimeStage/Ui.h` and docs, and regression coverage updates in `tests/unit/test_interaction.cpp`, `tests/unit/test_widget_visual_facility.cpp`, and `tests/unit/test_api_ergonomics.cpp`

- ☑ [51] Guarantee zero-theme-setup readable defaults.
  - default widget rendering is legible without app-level palette or color configuration
  - `examples/primestage_widgets.cpp` uses no custom theme/palette setup and remains readable
  - delivered by canonical default-theme bootstrap in `src/PrimeStage.cpp` plus source-guard/readability regression coverage in `tests/unit/test_api_ergonomics.cpp`

- ☑ [22] Define PrimeStage API ergonomics guidelines.
  - document what belongs in PrimeStage vs what user code should never have to implement
  - include examples for state ownership, interaction wiring, and focus semantics
  - delivered in `docs/api-ergonomics-guidelines.md` with regression coverage in `tests/unit/test_api_ergonomics.cpp`

- ☑ [11] Add a uniform interactive-control contract across widgets.
  - include first-class state + callbacks for toggle/checkbox/tabs/dropdown (`checked`/`selectedIndex`/`onChanged`)
  - ensure keyboard behavior is built-in for all interactive controls
  - define click + keyboard activation semantics (`Enter`/`Space`) consistently for button-like controls
  - acceptance: demo widgets no longer manually mutate app state from raw `PrimeFrame::Event` handlers
  - delivered by widget callback/state API updates in `include/PrimeStage/Ui.h`, built-in interaction handling in `src/PrimeStage.cpp`, and demo migration in `examples/primestage_widgets.cpp`

- ☑ [18] Make tabs and dropdown first-class interactive widgets.
  - add explicit callbacks (`onTabChanged`, `onOpened`, `onSelected`) and keyboard behavior
  - avoid attaching external pointer handlers to tab/dropdown child nodes
  - delivered by explicit callback APIs in `include/PrimeStage/Ui.h`, built-in interaction in `src/PrimeStage.cpp`, and demo/test migrations in `examples/primestage_widgets.cpp` and `tests/unit/test_tabs_dropdown.cpp`

- ☑ [12] Remove `PrimeFrame` callback plumbing from app-level widget usage.
  - provide PrimeStage-level event binding APIs so user code does not mutate node callbacks directly
  - acceptance: no `node->callbacks = ...` / callback-composition helpers needed in `examples/primestage_widgets.cpp`
  - delivered via widget-spec callbacks in `examples/primestage_widgets.cpp` with regression guard in `tests/unit/test_api_ergonomics.cpp`

- ☑ [19] Add core callback-composition utilities in PrimeStage.
  - expose safe helper APIs for extending existing widget callbacks (`onEvent`, `onFocus`, `onBlur`) without clobbering previous behavior
  - avoid ad-hoc callback composition in app code
  - delivered via `appendNodeOnEvent`, `appendNodeOnFocus`, and `appendNodeOnBlur` in `include/PrimeStage/Ui.h`/`src/PrimeStage.cpp` with regression coverage in `tests/unit/test_api_ergonomics.cpp`

- ☑ [13] Add stable widget identity and reconciliation support.
  - preserve focus/selection/caret across rebuilds without manual restore bookkeeping
  - acceptance: remove manual focus-target restore state from `examples/primestage_widgets.cpp`
  - delivered via `WidgetIdentityReconciler` in `include/PrimeStage/Ui.h`/`src/PrimeStage.cpp`, demo migration in `examples/primestage_widgets.cpp`, and regression coverage in `tests/unit/test_widget_identity.cpp` + `tests/unit/test_api_ergonomics.cpp`

- ☑ [17] Guarantee focus-ring compositing order.
  - ensure focus visuals render last/on top of selection/highlight primitives for all widgets
  - add regression tests that fail if selection highlight can draw above focus ring
  - delivered by explicit overlay top-order preservation in `src/PrimeStage.cpp` and flatten-order regression tests in `tests/unit/test_focus_compositing.cpp`

- ☑ [15] Add theme-level semantic defaults for interaction visuals.
  - define default focus ring styling for all focusable controls
  - reduce per-widget token opt-in for focus visuals
  - acceptance: focus ring appears for all focusable widgets without setting `focusStyle` per widget
  - delivered by semantic focus-style resolution in `src/PrimeStage.cpp` across focusable widget builders with regression coverage in `tests/unit/test_focus_widgets.cpp`

- ☑ [14] Add a PrimeStage input bridge layer for host integrations.
  - map host/platform events to widget events centrally (avoid per-app low-level event translation and keycode wiring)
  - replace raw numeric keycode handling with symbolic key enums in app-facing APIs
  - acceptance: app event loop no longer translates `PrimeHost` events to `PrimeFrame::Event` inline
  - delivered via `include/PrimeStage/InputBridge.h`, example migration in `examples/primestage_widgets.cpp`, and regression coverage in `tests/unit/test_input_bridge.cpp` + `tests/unit/test_api_ergonomics.cpp`

- ☑ [16] Add integration tests for real interaction flows.
  - cover click/type/rebuild/focus-retention scenarios in example-like app compositions
  - include focus-visual assertions (not only focused-node id assertions)
  - include explicit focus contract matrix: interaction-enabled widgets focusable by default; selectable text not focusable
  - delivered via `tests/unit/test_integration_flows.cpp` (end-to-end interaction/rebuild focus retention and focus-contract matrix coverage) and `CMakeLists.txt` test target updates

- ☑ [20] Add app-level interaction regression suite for the widgets demo.
  - scenario coverage: click-to-focus, tab traversal, value changes, text editing, tab/page switches, rebuild stability
  - include wheel + scrollbar interaction scenarios for overflow/scrollable regions
  - run in CI via `scripts/compile.sh --test`
  - delivered via `tests/unit/test_widgets_demo_regression.cpp` (demo-like page-flow/rebuild regression and wheel+scrollbar interaction coverage) and `CMakeLists.txt` test target updates


### P1 (Do Next)

- ☑ [23] Add a PrimeStage app runtime helper for frame lifecycle.
  - centralize rebuild/layout/render scheduling and dirty-flag handling
  - avoid ad-hoc `needsRebuild` / `needsLayout` / `needsFrame` orchestration in example apps
  - delivered via `include/PrimeStage/AppRuntime.h` (`FrameLifecycle` helper), migration in `examples/primestage_widgets.cpp`, and regression coverage in `tests/unit/test_app_runtime.cpp` + `tests/unit/test_api_ergonomics.cpp`

- ☑ [24] Resolve and record design decisions from open questions.
  - decide table vs specialized list strategy
  - decide window-chrome ownership (auto-generated vs composed widgets)
  - decide and document patch-operation safety whitelist
  - delivered via `docs/design-decisions.md`, `docs/prime-stage-design.md` alignment updates, and regression checks in `tests/unit/test_api_ergonomics.cpp`

- ☑ [21] Sync docs with current build/test workflow and API.
  - update `README.md` to promote `scripts/compile.sh` and `scripts/compile.sh --test`
  - reconcile `docs/prime-stage-design.md` with current API names/signatures (`TextField` vs draft `EditBox`, callback model, focus behavior)
  - delivered via README workflow updates, design-doc API/focus reconciliation in `docs/prime-stage-design.md`, and regression checks in `tests/unit/test_api_ergonomics.cpp`

- ☑ [25] Add visual regression coverage for interaction states.
  - add snapshot tests for focus/hover/pressed/selection layering
  - ensure regressions are catchable without manual runtime inspection
  - delivered via `tests/unit/test_visual_regression.cpp` and golden snapshot baseline `tests/snapshots/interaction_visuals.snap` with CMake wiring

- ☑ [31] Add CI/presubmit build matrix.
  - run `scripts/compile.sh --test` in at least `RelWithDebInfo` and `Release`
  - add coverage for `PRIMESTAGE_HEADLESS_ONLY=ON` and `PRIMESTAGE_ENABLE_PRIMEMANIFEST=OFF` compatibility path
  - fail fast on test regressions in the interaction/focus suites
  - delivered via `.github/workflows/presubmit.yml` (release/relwithdebinfo matrix + headless compatibility job) and workflow regression checks in `tests/unit/test_api_ergonomics.cpp`

- ☑ [32] Add toolchain quality gates.
  - add sanitizer configuration (`ASan`/`UBSan`) for tests
  - add static checks for warnings in project-owned code paths
  - keep third-party warnings isolated (do not block local code quality gate)
  - delivered via CMake quality-gate options in `CMakeLists.txt`, CLI wiring in `scripts/compile.sh`, presubmit `toolchain-quality` job in `.github/workflows/presubmit.yml`, and regression checks in `tests/unit/test_api_ergonomics.cpp`

- ☑ [37] Add API validation and diagnostics for widget specs.
  - detect invalid/inconsistent spec inputs (negative sizes, impossible ranges, invalid selection indices)
  - provide debug diagnostics/assertions and safe runtime fallbacks
  - add unit tests for validation behavior
  - delivered via spec sanitizers/index clamps in `src/PrimeStage.cpp`, runtime validation coverage in `tests/unit/test_spec_validation.cpp`, and source/docs guard checks in `tests/unit/test_api_ergonomics.cpp`

- ☑ [27] Define and implement disabled/read-only widget semantics.
  - add common `enabled`/`readOnly` behavior contract for interactive widgets where applicable
  - ensure disabled controls are not focusable/clickable and have consistent visuals
  - delivered via widget-spec API updates in `include/PrimeStage/Ui.h`, disabled/read-only interaction + scrim semantics in `src/PrimeStage.cpp`, and regression coverage in `tests/unit/test_interaction.cpp` + `tests/unit/test_api_ergonomics.cpp`

- ☑ [28] Expose focus-order API at widget level.
  - add user-facing `tabIndex`/focus-order controls without requiring direct `PrimeFrame` node access
  - include tests for deterministic tab order across mixed widget sets
  - delivered via `tabIndex` fields in focusable specs in `include/PrimeStage/Ui.h`, focus-order/clamp wiring in `src/PrimeStage.cpp`, and mixed-widget tab-order regression coverage in `tests/unit/test_focus_widgets.cpp` + `tests/unit/test_api_ergonomics.cpp`

- ☑ [29] Define controlled vs uncontrolled widget state model.
  - document when PrimeStage owns transient widget state vs when user code provides it
  - reduce full-app state bookkeeping for common controls (toggle/checkbox/dropdown/tabs)
  - delivered via state-backed widget APIs (`ToggleState`/`CheckboxState`/`TabsState`/`DropdownState`) in `include/PrimeStage/Ui.h`, runtime state ownership wiring in `src/PrimeStage.cpp`, and coverage in `tests/unit/test_interaction.cpp`, `tests/unit/test_tabs_dropdown.cpp`, and `tests/unit/test_api_ergonomics.cpp` with docs updates in `docs/api-ergonomics-guidelines.md` and `docs/prime-stage-design.md`

- ☑ [33] Normalize cross-platform input semantics in PrimeStage.
  - standardize key naming/translation (no hard-coded numeric key constants in examples)
  - document and test pointer scroll direction/units consistently across host backends
  - delivered via shared `KeyCode` semantics in `include/PrimeStage/Ui.h`, input normalization updates in `include/PrimeStage/InputBridge.h`, internal/test migration away from raw key literals in `src/PrimeStage.cpp` + input tests, and docs/test guards in `docs/api-ergonomics-guidelines.md`, `docs/prime-stage-design.md`, and `tests/unit/test_api_ergonomics.cpp`

- ☑ [35] Add IME/composition support plan for text input controls.
  - define API for composition start/update/commit/cancel
  - add tests for non-ASCII and composition-heavy input workflows
  - delivered via composition API planning types in `include/PrimeStage/Ui.h`, implementation plan in `docs/ime-composition-plan.md` with design/guideline links, and UTF-8/composition-like workflow regression coverage in `tests/unit/test_text_field.cpp` + source/docs guards in `tests/unit/test_api_ergonomics.cpp`

- ☑ [36] Add accessibility semantics roadmap.
  - define widget role/state metadata model (for future HTML/backend accessibility integration)
  - ensure focus order and activation semantics map cleanly to accessibility expectations
  - delivered via accessibility metadata model types in `include/PrimeStage/Ui.h`, roadmap doc `docs/accessibility-semantics-roadmap.md`, design/guideline links in docs, and behavior/source guards in `tests/unit/test_interaction.cpp` + `tests/unit/test_api_ergonomics.cpp`

- ☑ [38] Add performance benchmarks and budgets.
  - benchmark rebuild/layout/render cost for representative widget scenes
  - set target budgets for interaction-heavy flows (typing, drag, wheel scroll)
  - gate regressions in CI with trend/budget checks where feasible
  - delivered via benchmark harness `tests/perf/PrimeStage_benchmarks.cpp`, budget thresholds in `tests/perf/perf_budgets.txt`, CLI workflow wiring in `scripts/compile.sh`, CI budget gate in `.github/workflows/presubmit.yml`, documentation in `docs/performance-benchmarks.md`, and source/docs regression guards in `tests/unit/test_api_ergonomics.cpp`

- ☑ [39] Add deterministic visual-test harness.
  - stabilize snapshot rendering inputs (fonts/theme/scale) to reduce platform-dependent drift
  - define golden update workflow and failure triage guidance
  - delivered via deterministic harness helpers in `tests/unit/visual_test_harness.h`, visual regression migration in `tests/unit/test_visual_regression.cpp` with harness metadata block in `tests/snapshots/interaction_visuals.snap`, workflow/triage doc in `docs/visual-test-harness.md`, and source/docs guard coverage in `tests/unit/test_api_ergonomics.cpp`

- ☑ [40] Define API evolution and compatibility policy.
  - document semver expectations and deprecation process for spec/callback changes
  - provide migration notes when replacing draft APIs (for example `EditBox` naming in design docs)
  - delivered via compatibility policy doc `docs/api-evolution-policy.md` (semver/deprecation/checklist + `EditBox` -> `TextField` migration notes), design/guideline/readme links in docs, and regression guards in `tests/unit/test_api_ergonomics.cpp`

- ☑ [41] Add install/export/package support for PrimeStage.
  - add `install()` rules and CMake package exports for library + public headers
  - verify consumer integration with `find_package(PrimeStage)` style workflow
  - delivered via install/export/package config wiring in `CMakeLists.txt` + `cmake/PrimeStageConfig.cmake.in`, consumer smoke assets in `tests/cmake/find_package_smoke/*` with ctest driver `tests/cmake/run_find_package_smoke.cmake`, packaging docs in `docs/cmake-packaging.md`, and regression guards in `tests/unit/test_api_ergonomics.cpp`

- ☑ [42] Define callback reentrancy/threading guarantees.
  - document which callbacks may trigger rebuild/layout and in what execution context
  - add guardrails/tests for callback-triggered state changes to avoid reentrancy bugs
  - delivered via callback contract doc `docs/callback-reentrancy-threading.md`, docs alignment in `docs/api-ergonomics-guidelines.md` + `docs/prime-stage-design.md`, runtime reentrancy suppression guardrails in `src/PrimeStage.cpp`/`include/PrimeStage/Ui.h` for callback composition helpers, and regression coverage in `tests/unit/test_api_ergonomics.cpp`

- ☑ [48] Define data ownership/lifetime contracts in public specs.
  - document safe usage rules for `std::string_view` fields and callback captures
  - eliminate dangling-view hazards by copying where required or introducing owned-string alternatives
  - add tests that exercise rebuilds/callbacks with short-lived source strings
  - delivered via ownership contract doc `docs/data-ownership-lifetime.md`, guideline/design/readme/agents alignment, owned-row backing for table callback payloads in `src/PrimeStage.cpp`, and short-lived source + docs/source guard coverage in `tests/unit/test_interaction.cpp` and `tests/unit/test_api_ergonomics.cpp`

- ☑ [43] Improve render API diagnostics and failure reporting.
  - replace opaque `bool` failures with structured error/status information
  - expose actionable error context for PNG write/layout/render failures
  - delivered via structured render diagnostics API (`RenderStatusCode`/`RenderStatus` + `renderStatusMessage`) in `include/PrimeStage/Render.h` and `src/Render.cpp`, benchmark-callsite migration in `tests/perf/PrimeStage_benchmarks.cpp`, render status regression tests in `tests/unit/test_render.cpp`, docs in `docs/render-diagnostics.md` with README/design/agents alignment, and source/docs guard coverage in `tests/unit/test_api_ergonomics.cpp`

- ☑ [44] Remove renderer style heuristics tied to theme color indices.
  - replace color-index-based rounded-corner inference with explicit style metadata
  - ensure renderer output is deterministic under theme changes
  - delivered via explicit `CornerStyleMetadata` in `include/PrimeStage/Render.h`, geometry/metadata-driven radius resolution in `src/Render.cpp` (theme-color heuristics removed), deterministic theme-change regression coverage in `tests/unit/test_render.cpp`, and docs/guidance alignment in `docs/render-diagnostics.md`, `docs/prime-stage-design.md`, and `AGENTS.md`

- ☑ [45] Add render-path test coverage.
  - add tests for `renderFrameToTarget` and `renderFrameToPng` success/failure paths
  - include headless (`PRIMESTAGE_ENABLE_PRIMEMANIFEST=OFF`) behavior expectations
  - delivered via expanded render-path regression coverage in `tests/unit/test_render.cpp` (target/png overload success paths, PNG write failure path, and explicit headless expectations), source/docs guard updates in `tests/unit/test_api_ergonomics.cpp`, and render diagnostics coverage notes in `docs/render-diagnostics.md`

- ☑ [46] Establish single-source versioning.
  - derive runtime version string from project version metadata
  - add checks to prevent drift between CMake version, `getVersionString()`, and tests
  - delivered via generated CMake version header template `cmake/PrimeStageVersion.h.in`, configure/include wiring in `CMakeLists.txt`, runtime version derivation in `src/PrimeStage.cpp`, CMake-sourced version assertions in `tests/unit/test_sanity.cpp`, source/docs guard coverage in `tests/unit/test_api_ergonomics.cpp`, and contributor guidance update in `AGENTS.md`

- ☑ [47] Make dependency resolution reproducible.
  - avoid floating `master` defaults for fetched dependencies in release-grade workflows
  - document pin/update policy for PrimeFrame/PrimeHost/PrimeManifest revisions
  - delivered via pinned FetchContent defaults in `CMakeLists.txt` (`PRIMEFRAME_GIT_TAG`, `PRIMEHOST_GIT_TAG`, `PRIMESTAGE_PRIMEMANIFEST_GIT_TAG`), dependency pin/update policy doc `docs/dependency-resolution-policy.md`, README/agents alignment, and source/docs guard coverage in `tests/unit/test_api_ergonomics.cpp`

- ☑ [49] Add property/fuzz testing for input and focus state machines.
  - generate mixed pointer/keyboard/text event sequences and assert invariants (no crashes, valid focus, stable selection bounds)
  - include regression corpus for previously observed focus/callback bugs
  - delivered via deterministic property/fuzz suite `tests/unit/test_state_machine_fuzz.cpp` (seeded mixed-event generation + invariant checks + regression corpus), test-target wiring in `CMakeLists.txt`, source/docs guard coverage in `tests/unit/test_api_ergonomics.cpp`, and reproducibility guidance in `AGENTS.md`

- ☑ [50] Make example apps canonical API consumers.
  - ensure examples avoid direct `PrimeFrame` internals except where explicitly documented as advanced
  - add review checklist/tests to prevent examples from reintroducing callback/focus workarounds
  - delivered via canonical-consumer checklist doc `docs/example-app-consumer-checklist.md`, advanced-exception inline markers in `examples/primestage_widgets.cpp`, docs/guidance alignment in `README.md`, `docs/prime-stage-design.md`, and `AGENTS.md`, and regression guard coverage in `tests/unit/test_api_ergonomics.cpp`

- ☑ [7] Implement window builder (stateless) + callback wiring.
  - delivered via `WindowSpec`/`WindowCallbacks`/`Window` API additions and `UiNode::createWindow(...)` in `include/PrimeStage/Ui.h` + `src/PrimeStage.cpp` (stateless move/resize/focus callback wiring), regression coverage in `tests/unit/test_interaction.cpp` + `tests/unit/test_api_ergonomics.cpp`, and guidance updates in `docs/prime-stage-design.md`, `docs/api-ergonomics-guidelines.md`, and `AGENTS.md`

- ☑ [10] Final pass: validate naming rules, add minimal API docs, and run tests.
  - delivered via minimal symbol reference `docs/minimal-api-reference.md` with README/design/agents alignment, automated naming/doc guard coverage in `tests/unit/test_api_ergonomics.cpp` (public-header naming + API-reference checks), and full validation runs through `scripts/compile.sh --test` and `scripts/compile.sh`


### P2 (Foundational Cleanup / Backlog)

- ☑ [26] Add patch-first update path for high-frequency text/selection edits.
  - avoid full-scene rebuild on every text/caret update where structural changes are not required
  - delivered via in-place `TextField` visual patching in `src/PrimeStage.cpp` (`TextFieldPatchState` + callback-driven primitive updates), no-rebuild regression coverage in `tests/unit/test_text_field.cpp`, docs/runtime guidance alignment in `docs/api-ergonomics-guidelines.md`, `docs/prime-stage-design.md`, `docs/minimal-api-reference.md`, `AGENTS.md`, and patch-first usage checks in `tests/unit/test_api_ergonomics.cpp` + `examples/primestage_widgets.cpp`

- ☑ [30] Expand patch-first updates beyond text fields.
  - support non-structural interaction updates (toggle/checkbox/slider/progress visual state) without full rebuilds
  - delivered via in-place toggle/checkbox visual patching and state-backed progress-bar patch callbacks in `src/PrimeStage.cpp`, regression coverage in `tests/unit/test_interaction.cpp`, and patch-first guidance updates in `docs/api-ergonomics-guidelines.md`, `docs/prime-stage-design.md`, `docs/minimal-api-reference.md`, `tests/unit/test_api_ergonomics.cpp`, and `AGENTS.md`

- ☑ [34] Keep generated/build artifacts out of source control workflows.
  - document expected ignored paths for build outputs and snapshots
  - add cleanup guidance/script for local dev loops
  - delivered via `scripts/clean.sh`, artifact policy documentation in `docs/build-artifact-hygiene.md`, ignore-list updates in `.gitignore`, workflow guidance in `README.md` + `AGENTS.md`, and regression checks in `tests/unit/test_api_ergonomics.cpp`

- ☑ [1] Establish core ids, enums, and shared widget specs.
  - delivered via `WidgetKind` + `widgetKindName(...)`, `WidgetIdentityId` + `widgetIdentityId(...)`, shared spec bases (`WidgetSpec`/`EnableableWidgetSpec`/`FocusableWidgetSpec`) and spec adoption in `include/PrimeStage/Ui.h`, reconciler id-overload support in `src/PrimeStage.cpp`, regression coverage in `tests/unit/test_api_ergonomics.cpp`, and docs alignment in `docs/minimal-api-reference.md`, `docs/api-ergonomics-guidelines.md`, `docs/prime-stage-design.md`, and `AGENTS.md`
- ☑ [2] Define callback table + RAII callback handle.
  - delivered via `NodeCallbackTable` + move-only `NodeCallbackHandle` API in `include/PrimeStage/Ui.h`, install/restore implementation in `src/PrimeStage.cpp`, RAII/move/teardown regression coverage in `tests/unit/test_api_ergonomics.cpp`, and callback-guidance updates in `docs/minimal-api-reference.md`, `docs/callback-reentrancy-threading.md`, `docs/api-ergonomics-guidelines.md`, `docs/prime-stage-design.md`, and `AGENTS.md`
- ☑ [3] Implement `UiNode` fluent builder for `Frame` authoring.
  - delivered via `UiNode::with(...)`, broad `createX(spec, fn)` fluent overloads including typed
    `createScrollView(...)`/`createWindow(...)` variants in `include/PrimeStage/Ui.h`, runtime
    fluent-authoring regression coverage in `tests/unit/test_api_ergonomics.cpp`, and builder docs
    updates in `docs/minimal-api-reference.md`, `docs/prime-stage-design.md`, and
    `docs/api-ergonomics-guidelines.md`
- ☑ [4] Implement widget creation helpers (panel, label, divider, spacer).
  - delivered via helper-spec normalization in `src/PrimeStage.cpp` (`createPanel`,
    `createLabel`, `createDivider`, `createSpacer`) with clamped helper inputs, focused helper
    validation coverage in `tests/unit/test_spec_validation.cpp`, and helper-overload reference
    updates in `docs/minimal-api-reference.md`
- ☑ [5] Implement interactive widgets (button, edit box, toggle, checkbox, slider).
  - delivered via interactive helper overloads in `include/PrimeStage/Ui.h`/`src/PrimeStage.cpp`
    (`createButton`, state-backed `createTextField` as the edit-box helper path, `createToggle`,
    `createCheckbox`, `createSlider`), overload behavior/clamp regression coverage in
    `tests/unit/test_spec_validation.cpp`, and API reference + guard updates in
    `docs/minimal-api-reference.md` and `tests/unit/test_api_ergonomics.cpp`
- ☑ [6] Implement collection widgets (scroll view, list, table, tree).
  - delivered via collection helper overloads in `include/PrimeStage/Ui.h`/`src/PrimeStage.cpp`
    (`createScrollView(size, ...)`, `createTable(columns, rows, ...)`,
    `createTreeView(nodes, size)`), a new table-backed `ListSpec`/`createList(...)` API in
    `include/PrimeStage/Ui.h` with implementation in `src/PrimeStage.cpp`, regression coverage in
    `tests/unit/test_spec_validation.cpp`, and docs/guard updates in
    `docs/minimal-api-reference.md`, `docs/prime-stage-design.md`, and
    `tests/unit/test_api_ergonomics.cpp`
- ☑ [8] Add minimal unit tests for builder API.
  - delivered via dedicated minimal builder coverage in
    `tests/unit/test_builder_api.cpp` (nested fluent composition + non-materialized default
    builder diagnostics), with test-target wiring in `CMakeLists.txt`
- ☑ [9] Add example scene build demonstrating window + widgets.
  - delivered via new example app `examples/primestage_scene.cpp` (window + widget-scene
    composition and layout pass), example-target wiring in `CMakeLists.txt`, regression guards in
    `tests/unit/test_api_ergonomics.cpp`, and README example-binary documentation updates
